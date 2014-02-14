#include "G4CMPInterValleyScattering.hh"
      
#include "G4CMPDriftElectron.hh"
#include "G4CMPDriftHole.hh"

#include "G4PhononLong.hh"
#include "G4LatticeManager.hh"
#include "G4LatticePhysical.hh"

#include "G4Step.hh"
#include "G4StepPoint.hh"
#include "G4VParticleChange.hh"
#include "Randomize.hh"
#include "G4RandomDirection.hh"
#include "G4CMPValleyTrackMap.hh"

#include "G4VPhysicalVolume.hh"
#include "G4LogicalVolume.hh"
#include "G4FieldManager.hh"
#include "G4Field.hh"
#include "G4TransportationManager.hh"
#include "G4Geantino.hh"
#include "G4SystemOfUnits.hh"
#include "G4PhysicalConstants.hh"

#include "math.h"

G4CMPInterValleyScattering::G4CMPInterValleyScattering()
:G4VDiscreteProcess("InterValleyScattering")
{
  //E_0 from Edelweiss experiment LTD 14. This value is tuned for
  //EDELWEISS/CDMS crystals with small (~V/m) electric fields. It 
  //may still be fine for larger electric fields (~10-100V/m) 
  //however there is no experimental data as of now. 
  E_0_ED_203 = 217.0 ; // V/m
    
  if(verboseLevel>1){
    G4cout<<GetProcessName()<<" is created "<<G4endl;
  }
}

G4CMPInterValleyScattering::~G4CMPInterValleyScattering()
{ ; }

G4CMPInterValleyScattering::G4CMPInterValleyScattering(G4CMPInterValleyScattering& right)
: G4VDiscreteProcess(right)
{ ; }

G4double G4CMPInterValleyScattering::GetMeanFreePath(const G4Track& aTrack, G4double, G4ForceCondition* condition)
{
    G4StepPoint* stepPoint  = aTrack.GetStep()->GetPostStepPoint();
    G4double velocity = stepPoint->GetVelocity();
    *condition = NotForced;
    
    G4RotationMatrix trix;
    int valley = G4CMPValleyTrackMap::GetInstance()->GetValley(aTrack);
    
    switch(valley) {
    case 1: trix = G4RotationMatrix(-pi/4,-pi/4, pi/4); break;
    case 2: trix = G4RotationMatrix( pi/4,-pi/4,-pi/4); break;
    case 3: trix = G4RotationMatrix(-pi/4, pi/4, pi/4); break;
    case 4: trix = G4RotationMatrix( pi/4, pi/4,-pi/4); break;
    }

    normalToValley= G4AffineTransform(trix);
    valleyToNormal= G4AffineTransform(trix).Inverse();
    
    //G4VPhysicalVolume* pVol = aTrack.GetVolume();
    //G4LogicalVolume* lVol = pVol->GetLogicalVolume();
    
    G4FieldManager* fMan = G4TransportationManager::GetTransportationManager()->GetFieldManager();
    
    //If there is no field, there is no IV scattering... but then there
    //is no e-h transport either...
    if (!fMan->DoesFieldExist()) return DBL_MAX;

    //Getting the field.
    const G4Field* field = fMan->GetDetectorField();
    
    G4ThreeVector pos = aTrack.GetPosition();
    
    G4double  posVec[4];
    
    posVec[0] = pos.x();
    posVec[1] = pos.y();
    posVec[2] = pos.z();
    posVec[3] = 0;
    
    G4double fieldValue[6];
    
    field->GetFieldValue(posVec,fieldValue);
    
    G4ThreeVector fieldVector = G4ThreeVector(fieldValue[3]/(volt/m),
					      fieldValue[4]/(volt/m), 
					      fieldValue[5]/(volt/m));
    
    G4ThreeVector fieldVectorLLT = 
      normalToValley.TransformAxis(fieldVector).unit();
    G4ThreeVector fieldDirHV = G4ThreeVector(fieldVectorLLT.getX()*(1/1.2172),
					     fieldVectorLLT.getY()*(1/1.2172),
					     fieldVectorLLT.getZ()*(1/0.27559)
					     );
    
    G4double fieldMagHV      = fieldDirHV.mag();
    
    //setting the E-field parameter as outline in EDELSWEISS LTD-14.
    E_0_ED_201 = fieldMagHV; 
    
    G4double mfp = velocity * 6.72e-6 * 
      pow((E_0_ED_203 * E_0_ED_203 + abs(E_0_ED_201) * abs(E_0_ED_201)), 3.24/2.0 );
    
    return mfp;

}

G4VParticleChange* 
G4CMPInterValleyScattering::PostStepDoIt(const G4Track& aTrack, 
					 const G4Step& /*aStep*/) {

    aParticleChange.Initialize(aTrack);  

    //picking a new valley at random if IV-scattering process was triggered
    int valley = (int) (G4UniformRand()*4 + 1.0);
    
    //Assigning a new valley...
    G4CMPValleyTrackMap::GetInstance()->SetValley(aTrack, valley);

    G4RotationMatrix trix;
    switch(valley){
      case 1:
          trix = G4RotationMatrix(-pi/4, -pi/4, pi/4);
          break;
      case 2:
          trix = G4RotationMatrix(pi/4, -pi/4, -pi/4);
          break;
      case 3:
          trix = G4RotationMatrix(-pi/4, pi/4, pi/4);
          break;
      case 4:
          trix = G4RotationMatrix(pi/4, pi/4, -pi/4);
          break;
  }

    normalToValley = G4AffineTransform(trix);
    valleyToNormal = G4AffineTransform(trix).Inverse();
    
    G4ThreeVector DirXYZ = aTrack.GetMomentumDirection();  
    // G4double velocityXYZ = aStep.GetPostStepPoint()->GetVelocity();
    
    aParticleChange.ProposeMomentumDirection(DirXYZ);
    aParticleChange.ProposeEnergy(aTrack.GetKineticEnergy());  
    
    ResetNumberOfInteractionLengthLeft();    

    return &aParticleChange;

}

G4bool G4CMPInterValleyScattering::IsApplicable(const G4ParticleDefinition& aPD)
{
  
  return((&aPD==G4CMPDriftElectron::Definition()));
}
