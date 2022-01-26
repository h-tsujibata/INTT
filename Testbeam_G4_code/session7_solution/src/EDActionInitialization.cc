//
// ********************************************************************
// * License and Disclaimer                                           *
// *                                                                  *
// * The  Geant4 software  is  copyright of the Copyright Holders  of *
// * the Geant4 Collaboration.  It is provided  under  the terms  and *
// * conditions of the Geant4 Software License,  included in the file *
// * LICENSE and available at  http://cern.ch/geant4/license .  These *
// * include a list of copyright holders.                             *
// *                                                                  *
// * Neither the authors of this software system, nor their employing *
// * institutes,nor the agencies providing financial support for this *
// * work  make  any representation or  warranty, express or implied, *
// * regarding  this  software system or assume any liability for its *
// * use.  Please see the license in the file  LICENSE  and URL above *
// * for the full disclaimer and the limitation of liability.         *
// *                                                                  *
// * This  code  implementation is the result of  the  scientific and *
// * technical work of the GEANT4 collaboration.                      *
// * By using,  copying,  modifying or  distributing the software (or *
// * any work based  on the software)  you  agree  to acknowledge its *
// * use  in  resulting  scientific  publications,  and indicate your *
// * acceptance of all terms of the Geant4 Software license.          *
// ********************************************************************
//
// $Id: EDActionInitialization.cc 68058 2013-03-13 14:47:43Z gcosmo $
//
/// \file EDActionInitialization.cc
/// \brief Implementation of the EDActionInitialization class

#include "EDActionInitialization.hh"

EDActionInitialization::EDActionInitialization( INTTMessenger* INTT_mess, EDDetectorConstruction* fDetConstruction_arg )
  : G4VUserActionInitialization(),
    fDetConstruction( fDetConstruction_arg ),
    INTT_mess_( INTT_mess )
{}

EDActionInitialization::~EDActionInitialization()
{}

void EDActionInitialization::BuildForMaster() const
{

  OutputManager* output = new OutputManager();
  
  SetUserAction(
		new EDRunAction( INTT_mess_,
				new EDPrimaryGeneratorAction( INTT_mess_ ),
				new EDEventAction(),
				output
				)
		);
}

void EDActionInitialization::Build() const
{
  auto pga = new EDPrimaryGeneratorAction( INTT_mess_ );  
  SetUserAction( pga );

  OutputManager* output = new OutputManager();
  auto event = new EDEventAction;
  SetUserAction( event );
  SetUserAction(new EDRunAction( INTT_mess_, pga, event, output ) );
  SetUserAction(new TrackingAction( pga ) );
  SetUserAction( new SteppingAction(fDetConstruction) );
}  
