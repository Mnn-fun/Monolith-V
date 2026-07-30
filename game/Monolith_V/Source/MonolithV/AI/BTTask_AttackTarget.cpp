#include "BTTask_AttackTarget.h"
#include "AIController.h"
#include "EnemyBaseCharacter.h"
#include "../Combat/MeleeHitboxComponent.h"
#include "GoliathCharacter.h"

UBTTask_AttackTarget::UBTTask_AttackTarget()
{
	NodeName = "Attack Target (GAS)";
}

EBTNodeResult::Type UBTTask_AttackTarget::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController) return EBTNodeResult::Failed;

	AEnemyBaseCharacter* Enemy = Cast<AEnemyBaseCharacter>(AIController->GetPawn());
	if (!Enemy) return EBTNodeResult::Failed;

	UMeleeHitboxComponent* Hitbox = Enemy->MeleeHitboxComponent;
	if (!Hitbox) return EBTNodeResult::Failed;

	// If this is a Goliath, use its custom multi-attack system
	if (AGoliathCharacter* Goliath = Cast<AGoliathCharacter>(Enemy))
	{
		// Since Goliath doesn't use the MeleeHitbox component's cooldown natively for its complex attacks, 
		// we check it manually here for consistency
		if (Hitbox && !Hitbox->CanAttack()) return EBTNodeResult::Failed;
		
		bool bHitSomething = Goliath->ExecuteGoliathAttack();
		
		// Update the hitbox timer so CanAttack works for the Goliath too
		if (Hitbox) Hitbox->ExecuteAttack(); // Just to reset the cooldown timer internally
		
		return bHitSomething ? EBTNodeResult::Succeeded : EBTNodeResult::Failed;
	}
	else
	{
		// Standard enemies use the shared Melee Hitbox Component
		if (!Hitbox->CanAttack()) return EBTNodeResult::Failed;
		bool bHitSomething = Hitbox->ExecuteAttack();
		return bHitSomething ? EBTNodeResult::Succeeded : EBTNodeResult::Failed;
	}
}
