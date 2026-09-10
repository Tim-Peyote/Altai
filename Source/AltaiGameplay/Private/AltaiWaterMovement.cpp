#include "AltaiWaterMovement.h"
#include "GameFramework/Character.h"
float UAltaiWaterMovement::GetMaxSpeed() const{return WaterActive?WaterSpeed:Super::GetMaxSpeed();}
void UAltaiWaterMovement::PhysCustom(float Dt,int32 Iterations)
{
 if(!WaterActive){Super::PhysCustom(Dt,Iterations);return;}
 if(Dt<MIN_TICK_TIME)return;
 // Fluid drag and finite acceleration, rather than teleporting to a depth or speed.
 if(PassiveSink){
  Acceleration=FVector::ZeroVector;
  Velocity=FMath::VInterpTo(Velocity,FVector(0,0,-18),Dt,1.5f);
 }else{
 Acceleration=Acceleration.GetClampedToMaxSize(360.f);
 if(Submerged){Acceleration+=FVector::UpVector*VerticalIntent*360.f;
  // C/Space are independent vertical inputs, so no WASD analog value is required.
  AnalogInputModifier=FMath::Max(AnalogInputModifier,FMath::Abs(VerticalIntent));}
 Acceleration=Acceleration.GetClampedToMaxSize(360.f);
 CalcVelocity(Dt,1.15f,true,90.f);
 if(!Submerged)Velocity.Z=FMath::FInterpTo(Velocity.Z,FMath::Clamp((WaterHeight-25.f-UpdatedComponent->GetComponentLocation().Z)*3.f,-100.f,100.f),Dt,4.f);
 Velocity=Velocity.GetClampedToMaxSize(WaterSpeed);
 }
 const FVector Start=UpdatedComponent->GetComponentLocation();FHitResult Hit;
 SafeMoveUpdatedComponent(Velocity*Dt,UpdatedComponent->GetComponentQuat(),true,Hit);
 if(Hit.IsValidBlockingHit()){HandleImpact(Hit,Dt,Velocity*Dt);SlideAlongSurface(Velocity*Dt,1.f-Hit.Time,Hit.Normal,Hit,true);}
 Velocity=(UpdatedComponent->GetComponentLocation()-Start)/Dt;
}
