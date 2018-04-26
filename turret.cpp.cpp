// Fill out your copyright notice in the Description page of Project Settings.

#include "AITurret.h"
#include "EngineUtils.h"
#include "Engine.h"
#include <string> 

// Sets default values
AAITurret::AAITurret()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.

	//sets the default values for most of the variables
	PrimaryActorTick.bCanEverTick = true;
	//default scene componet
	this->SceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComponent"));
	//default scene RootComponent
	this->RootComponent = SceneComponent;
	//default scene RootComponent
	this->TurretMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TurretMesh"));
	//default scene RootComponent
	this->rotationRate = FRotator(0.0, 180.0f, 0.0f);
	//default TurretMesh
	this->TurretMesh->AttachTo(RootComponent);
	//default health is 100
	this->health = 100.0f;
	//default turret range is 1000
	this->turretRange = 1000.0f;
	//default team to attack is 1
	this->teamToAttack = 1;
	//default timeToTakeOverTurret is 10 seconds
	this->timeToTakeOverTurret = 10;
	//default team taking over turret is no one
	this->teamTakingTurret = 0;
	//default currentllyAttackingTeam is equal to the team to attack
	this->currentllyAttackingTeam = teamToAttack;
	//default beingTaken is false
	this->beingTaken = false;
	//default target is null
	this->target = nullptr;
	//default disabled is false
	this->disabled = false;
	//default idle is false
	this->idle = false;
	this->callTimer = true;
	//default turret neatral state
	this->turretIsNeutral = false;
}

// Called when the game starts or when spawned
void AAITurret::BeginPlay()
{
	Super::BeginPlay();
	capturePointDelay = timeToTakeOverTurret;
}

// Called every frame
void AAITurret::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	//if the turret is being taken it disables it self and stop moves and capture point is reduced 
	if (beingTaken)
	{
		DisableTurret();

		if(capturePointDelay <= 0)
			turretIsNeutral = true;
		if (capturePointDelay == timeToTakeOverTurret && turretIsNeutral)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("!!!ChangeingTeams!!!")));
			turretIsNeutral = false;
			
			if (teamTakingTurret == 1)
				teamToAttack = 2;
			else if(teamTakingTurret == 2)
				teamToAttack = 1;
			beingTaken = false;
		}

		if (turretIsNeutral == false)
		{
			if (callTimer)
			{
				GetWorld()->GetTimerManager().SetTimer(CapturePointDelayTimer, this, &AAITurret::reduseCapturePointDelay, 1.0f, false);
				callTimer = false;
			}
		}
		else
		{
			if (callTimer)
			{
				GetWorld()->GetTimerManager().SetTimer(CapturePointDelayTimer, this, &AAITurret::increasseCapturePointDelay, 1.0f, false);
				callTimer = false;
			}
		}
		
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("Team To Attack	: %i"), capturePointDelay));

	}
	else
	{
		//sets the capture point back if nobody is taking it over
		if (capturePointDelay != timeToTakeOverTurret && turretIsNeutral == false)
		{
			if (callTimer)
			{
				GetWorld()->GetTimerManager().SetTimer(CapturePointDelayTimer, this, &AAITurret::increasseCapturePointDelay, 1.0f, false);
				callTimer = false;
			}
		}
		else if (capturePointDelay != 0 && turretIsNeutral == true)
		{
			if (callTimer)
			{
				GetWorld()->GetTimerManager().SetTimer(CapturePointDelayTimer, this, &AAITurret::reduseCapturePointDelay, 1.0f, false);
				callTimer = false;
			}
		}



		//Looks for the target and if the target is not found the turret idles
		//If the target is found it starts to look at the player and if the player is hitable it starts to attack the target
		if (target == nullptr)
		{
			//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, FString::Printf(TEXT("IN here")));
			FindTarget(DeltaTime);
		}
		else if (target != nullptr)
		{
			//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("IN here")));
			FVector targetLocation = target->GetActorLocation();
			if (CheckIfTargetStillInRange() && teamToAttack == currentllyAttackingTeam)//checks if target in turret range
			{
				if (CheckIfTargetIsHitable(targetLocation))//checks if we can hit turret
					AttackTarget();//attacks turret
				else
					target = nullptr;
			}
			else
				target = nullptr;
		}
	}
}
//Looks for a target the turret can shoot at
void AAITurret::FindTarget(float timeDelta_)
{
	AShipTestWithAI* closestShip = nullptr;
	float closestShipDistance;

	for (TActorIterator<AShipTestWithAI> CharacterItr(GetWorld()); CharacterItr; ++CharacterItr)
	{
		AShipTestWithAI* Character = *CharacterItr;
		FVector vector_between_locations = GetActorLocation() - Character->GetActorLocation();
		float distance_between_locations = vector_between_locations.Size();
		FVector targetLocation = Character->GetActorLocation();

		//checks if the ship is the target team and that if its in range and hitable
		if (distance_between_locations < turretRange && teamToAttack == Character->GetTeam() && CheckIfTargetIsHitable(targetLocation))
		{
			if (closestShip == nullptr)
			{
				closestShip = Character;
				closestShipDistance = distance_between_locations;
			}
			else if (distance_between_locations < closestShipDistance)
			{
				closestShip = Character;
				closestShipDistance = distance_between_locations;
			}
		}
	}

	if (closestShip != nullptr)
	{
		target = closestShip;
		currentllyAttackingTeam = teamToAttack;
	}
	else
		TurretIdle(timeDelta_);
}
//raycasts to see if the target is hitable and if it is returns true
bool AAITurret::CheckIfTargetIsHitable(FVector targetPos_)
{
	FHitResult* hitResult = new FHitResult();
	FVector startTrace = this->GetActorLocation();
	FVector forwardVector = targetPos_ - startTrace;
	FVector endTrace = (forwardVector) + startTrace;
	FCollisionQueryParams* traceParms = new FCollisionQueryParams();
	traceParms->AddIgnoredActor(this); //tells the raycast to ignore the actor that is firing the raycast
	
	//stores the location of the ray hit
	FVector rayHitLocation;

	

	if (GetWorld()->LineTraceSingleByChannel(*hitResult, startTrace, endTrace, ECC_Visibility, *traceParms))
	{
		DrawDebugLine(GetWorld(), startTrace, endTrace, FColor(255, 0, 0), true);
		rayHitLocation = hitResult->Actor->GetActorLocation();
	}

	//Retuns true if target is the same as the one we are looking for else retuns false
	if (targetPos_ == rayHitLocation)
		return true;
	else
		return false;
}
//looks and shoots at the target
void AAITurret::AttackTarget()
{
	FVector targetLocation = target->GetActorLocation();
	FRotator PlayerRot = FRotationMatrix::MakeFromX(targetLocation - GetActorLocation()).Rotator();
	SetActorLocation(GetActorLocation());
	SetActorRotation(PlayerRot);
	weapon->Shoot();
}
//reduceses capture point by 1
void AAITurret::reduseCapturePointDelay()
{
	capturePointDelay--;
	callTimer = true;
}
//adds a point to the capture point by 1
void AAITurret::increasseCapturePointDelay()
{
	capturePointDelay++;
	callTimer = true;
}
//checks if the distance bettween the turret and ship is lower than the turret range
bool AAITurret::CheckIfTargetStillInRange()
{
	FVector targetLocation = target->GetActorLocation() - this->GetActorLocation();
	float distance_between_target = targetLocation.Size();
	if (distance_between_target > turretRange)
		return false;
	else
		return true;
}
//This is a methods the player can call to take over the turret
void AAITurret::takeOverTurret(bool isPlayerTakingOverTurret_, int team_)
{
	if (teamToAttack == team_ || teamToAttack == 0)
	{
		beingTaken = isPlayerTakingOverTurret_;
		teamTakingTurret = team_;
	}
}
//Disables the turret and makes it stop moving
void AAITurret::DisableTurret()
{
	//DisableTurret
	this->AddActorLocalRotation(this->rotationRate);
}
//Makes the turret rotate while looking for a target
void AAITurret::TurretIdle(float DeltaTimer_)
{
	//Sets the turretTo Idle
	this->AddActorLocalRotation(this->rotationRate * DeltaTimer_);
}