#include "PreRTS.h"

#include "GameLogic/FormationAI.h"
#include "Common/MessageStream.h"
#include "Common/PlayerList.h"
#include "GameLogic/AIStateMachine.h"
#include "GameLogic/AIPathfind.h"
#include "GameLogic/GameLogic.h"
#include "GameLogic/Module/AIUpdate.h"
#include "GameLogic/Object.h"
#include "WWMath/wwmath.h"

// GeneralsX @refactor Codex 02/10/2026 Isolate synchronized formation behavior from retail AI implementations.
// GeneralsX @feature Codex 26/09/2026 Reuse normal movement and save data for move-then-face orders.
class AIFormationMoveState : public AIMoveToState
{
	MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE(AIFormationMoveState, "AIFormationMoveState")
public:
	AIFormationMoveState(StateMachine *machine) : AIMoveToState(machine) { m_isMoveTo = false; }
};
EMPTY_DTOR(AIFormationMoveState)

// GeneralsX @feature Codex 26/09/2026 Use the normal turn rate without aiming at a drifting fixed point.
class AIFormationFaceState : public AIFaceState
{
	MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE(AIFormationFaceState, "AIFormationFaceState")
public:
	AIFormationFaceState(StateMachine *machine) : AIFaceState(machine, false) { }
	virtual StateReturnType update() override;
};
EMPTY_DTOR(AIFormationFaceState)

// GeneralsX @feature Codex 26/09/2026 Keep a constant heading while the locomotor finishes braking.
StateReturnType AIFormationFaceState::update()
{
	AIStateMachine *machine = static_cast<AIStateMachine *>(getMachine());
	if (machine->getGoalPathSize() != 2)
		return STATE_FAILURE;
	const Coord3D *destination = machine->getGoalPathPosition(0);
	const Coord3D *facingPoint = machine->getGoalPathPosition(1);
	Coord3D target = *getMachineOwner()->getPosition();
	target.x += facingPoint->x - destination->x;
	target.y += facingPoint->y - destination->y;
	machine->setGoalPosition(&target);
	return AIFaceState::update();
}

// GeneralsX @feature Codex 26/09/2026 Store both waypoints so deferred commands retain final facing.
void AICommandInterface::aiMoveToPositionAndFace(const Coord3D *pos, const Coord3D *facingPoint, CommandSourceType cmdSource)
{
	AICommandParms parms(AICMD_MOVE_TO_POSITION_AND_FACE, cmdSource);
	parms.m_pos = *pos;
	parms.m_coords.push_back(*pos);
	parms.m_coords.push_back(*facingPoint);
	aiDoCommand(&parms);
}


void AIStateMachine::defineFormationStates()
{
	// Face only after successful arrival; failure returns to idle.
	defineState(AI_FORMATION_MOVE, newInstance(AIFormationMoveState)(this), AI_FORMATION_FACE, AI_IDLE);
	defineState(AI_FORMATION_FACE, newInstance(AIFormationFaceState)(this), AI_IDLE, AI_IDLE);
}

void AIUpdateInterface::privateMoveToPositionAndFace(const AICommandParms *parms)
{
	if (!getObject()->isMobile() || parms->m_coords.size() != 2)
		return;
	getObject()->setFormationID(NO_FORMATION_ID);
	TheAI->pathfinder()->removeGoal(getObject());
	getStateMachine()->clear();
	setGoalPositionClipped(&parms->m_pos, parms->m_cmdSource);
	std::vector<Coord3D> path = parms->m_coords;
	getStateMachine()->setGoalPath(&path);
	m_blockedFrames = 0;
	m_isBlocked = FALSE;
	m_isBlockedAndStuck = FALSE;
	setLastCommandSource(parms->m_cmdSource);
	getStateMachine()->setState(AI_FORMATION_MOVE);
}

// GeneralsX @feature Codex 04/10/2026 Derive markers from live orders, without caching or changing simulation state.
void AIUpdateInterface::getFormationMarker(Coord3D &position, Coord3D &direction) const
{
	const Object *obj = getObject();
	position = *obj->getPosition();
	direction.x = WWMath::Cos(obj->getOrientation());
	direction.y = WWMath::Sin(obj->getOrientation());
	direction.z = 0.0f;
	const AIStateMachine *machine = getStateMachine();
	const StateID state = machine->getCurrentStateID();
	if ((state == AI_FORMATION_MOVE || state == AI_FORMATION_FACE) && machine->getGoalPathSize() == 2)
	{
		const Coord3D *destination = machine->getGoalPathPosition(0);
		const Coord3D *facing = machine->getGoalPathPosition(1);
		const Real dx = facing->x - destination->x, dy = facing->y - destination->y;
		const Real length = WWMath::Sqrt(dx * dx + dy * dy);
		if (length > 0.1f)
		{
			direction.x = dx / length;
			direction.y = dy / length;
		}
		// Keep the ordered slot even during a temporary avoidance move. On arrival, draw beneath the unit.
		if (state == AI_FORMATION_MOVE)
			position = *destination;
	}
	else if (state == AI_MOVE_TO && !machine->getGoalObject())
		position = *machine->getGoalPosition();
	else if (state == AI_FOLLOW_PATH && machine->getGoalPathSize() > 0)
		position = *machine->getGoalPathPosition(machine->getGoalPathSize() - 1);
	// Idle, Stop, attacks and other replacement orders use the current position/heading,
	// even if the state machine still contains coordinates from a completed formation order.
}

void executeFormationOrder(const GameMessage *msg)
{
	if (msg->getArgumentCount() != 3
		|| msg->getArgumentDataType(0) != ARGUMENTDATATYPE_OBJECTID
		|| msg->getArgumentDataType(1) != ARGUMENTDATATYPE_LOCATION
		|| msg->getArgumentDataType(2) != ARGUMENTDATATYPE_LOCATION)
		return;
	Object *obj = TheGameLogic->findObjectByID(msg->getArgument(0)->objectID);
	if (!obj || obj->getControllingPlayer() != ThePlayerList->getNthPlayer(msg->getPlayerIndex())
		|| obj->isEffectivelyDead() || obj->isContained() || !obj->isMobile()
		|| obj->isKindOf(KINDOF_AIRCRAFT) || !obj->getAIUpdateInterface())
		return;
	obj->releaseWeaponLock(LOCKED_TEMPORARILY);
	obj->getAIUpdateInterface()->aiMoveToPositionAndFace(
		&msg->getArgument(1)->location, &msg->getArgument(2)->location, CMD_FROM_PLAYER);
}
