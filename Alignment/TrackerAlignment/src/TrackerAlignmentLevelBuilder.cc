
#include "Alignment/TrackerAlignment/interface/TrackerAlignmentLevelBuilder.h"

// Original Author:  Max Stark
//         Created:  Wed, 10 Feb 2016 13:48:41 CET

#include "Alignment/CommonAlignment/interface/AlignableObjectId.h"

// these extern defined fields (see files TPBNameSpace.h etc.) hold some
// geometry-dependent values -> they will be set in this class
namespace align
{
  namespace tpb { extern std::vector<unsigned int> lpqc; }
  namespace tpe { extern unsigned int bpqd; }
  namespace tib { extern std::vector<unsigned int> sphs; }
}



//=============================================================================
//===   PUBLIC METHOD IMPLEMENTATION                                        ===
//=============================================================================

//_____________________________________________________________________________
TrackerAlignmentLevelBuilder
::TrackerAlignmentLevelBuilder(const TrackerTopology* trackerTopology) :
  trackerTopology_(trackerTopology)
{
}

//_____________________________________________________________________________
TrackerAlignmentLevelBuilder::
~TrackerAlignmentLevelBuilder()
{
}

//_____________________________________________________________________________
void TrackerAlignmentLevelBuilder
::addDetUnitInfo(const DetId& detId)
{
  int subdetId = detId.subdetId();

  switch (subdetId) {
    case PixelSubdetector::PixelBarrel: addPXBDetUnitInfo(detId); break;
    case PixelSubdetector::PixelEndcap: addPXEDetUnitInfo(detId); break;
    case StripSubdetector::TIB:         addTIBDetUnitInfo(detId); break;
    case StripSubdetector::TID:         addTIDDetUnitInfo(detId); break;
    case StripSubdetector::TOB:         addTOBDetUnitInfo(detId); break;
    case StripSubdetector::TEC:         addTECDetUnitInfo(detId); break;
  }
}

//_____________________________________________________________________________
align::vAlignmentLevels TrackerAlignmentLevelBuilder
::build()
{
  buildPXBAlignmentLevels();
  buildPXEAlignmentLevels();
  buildTIBAlignmentLevels();
  buildTIDAlignmentLevels();
  buildTOBAlignmentLevels();
  buildTECAlignmentLevels();

  levels_.push_back(pxb_);
  levels_.push_back(pxe_);
  levels_.push_back(tib_);
  levels_.push_back(tid_);
  levels_.push_back(tob_);
  levels_.push_back(tec_);

  return levels_;
}



//=============================================================================
//===   PRIVATE METHOD IMPLEMENTATION                                       ===
//=============================================================================

//_____________________________________________________________________________
void TrackerAlignmentLevelBuilder
::addPXBDetUnitInfo(const DetId& detId)
{
  auto layerID  = trackerTopology_->pxbLayer(detId);
  auto ladderID = trackerTopology_->pxbLadder(detId);
  auto moduleID = trackerTopology_->module(detId);

  if (pxbLaddersPerLayer_[layerID-1] < ladderID) {
    pxbLaddersPerLayer_[layerID-1] = ladderID;
  }

  pxbLayerIDs_. insert(layerID);
  pxbLadderIDs_.insert(ladderID);
  pxbModuleIDs_.insert(moduleID);
}

//_____________________________________________________________________________
void TrackerAlignmentLevelBuilder
::addPXEDetUnitInfo(const DetId& detId)
{
  auto sideID   = trackerTopology_->pxfSide(detId);
  auto diskID   = trackerTopology_->pxfDisk(detId);
  auto bladeID  = trackerTopology_->pxfBlade(detId);
  auto panelID  = trackerTopology_->pxfPanel(detId);
  auto moduleID = trackerTopology_->module(detId);

  pxeSideIDs_.  insert(sideID);
  pxeDiskIDs_.  insert(diskID);
  pxeBladeIDs_. insert(bladeID);
  pxePanelIDs_. insert(panelID);
  pxeModuleIDs_.insert(moduleID);
}

//_____________________________________________________________________________
void TrackerAlignmentLevelBuilder
::addTIBDetUnitInfo(const DetId& detId)
{
  auto sideID    = trackerTopology_->tibSide(detId);
  auto layerID   = trackerTopology_->tibLayer(detId);
  auto layerSide = trackerTopology_->tibOrder(detId);
  auto stringID  = trackerTopology_->tibString(detId);
  auto moduleID  = trackerTopology_->module(detId);

  if (layerSide == 1) {
    if (tidStringsInnerLayer_[layerID-1] < stringID) {
      tidStringsInnerLayer_[layerID-1] = stringID;
    }
  } else {
    if (tidStringsOuterLayer_[layerID-1] < stringID) {
      tidStringsOuterLayer_[layerID-1] = stringID;
    }
  }

  tibSideIDs_.  insert(sideID);
  tibLayerIDs_. insert(layerID);
  tibStringIDs_.insert(stringID);
  tibModuleIDs_.insert(moduleID);
}

//_____________________________________________________________________________
void TrackerAlignmentLevelBuilder
::addTIDDetUnitInfo(const DetId& detId)
{
  auto sideID   = trackerTopology_->tidSide(detId);
  auto wheelID  = trackerTopology_->tidWheel(detId);
  auto ringID   = trackerTopology_->tidRing(detId);
  auto moduleID = trackerTopology_->module(detId);

  // tidOrder
  tidSideIDs_.  insert(sideID);
  tidWheelIDs_. insert(wheelID);
  tidRingIDs_.  insert(ringID);
  tidModuleIDs_.insert(moduleID);
}

//_____________________________________________________________________________
void TrackerAlignmentLevelBuilder
::addTOBDetUnitInfo(const DetId& detId)
{
  auto layerID  = trackerTopology_->tobLayer(detId);
  auto sideID   = trackerTopology_->tobSide(detId);
  auto rodID    = trackerTopology_->tobRod(detId);
  auto moduleID = trackerTopology_->module(detId);

  tobLayerIDs_. insert(layerID);
  tobSideIDs_.  insert(sideID);
  tobRodIDs_.   insert(rodID);
  tobModuleIDs_.insert(moduleID);
}

//_____________________________________________________________________________
void TrackerAlignmentLevelBuilder
::addTECDetUnitInfo(const DetId& detId)
{
  auto sideID   = trackerTopology_->tecSide(detId);
  auto wheelID  = trackerTopology_->tecWheel(detId);
  auto petalID  = trackerTopology_->tecPetalNumber(detId);
  auto ringID   = trackerTopology_->tecRing(detId);
  auto moduleID = trackerTopology_->module(detId);

  tecSideIDs_.  insert(sideID);
  tecWheelIDs_. insert(wheelID);
  tecPetalIDs_. insert(petalID);
  tecRingIDs_.  insert(ringID);
  tecModuleIDs_.insert(moduleID);
}



//_____________________________________________________________________________
void TrackerAlignmentLevelBuilder
::buildPXBAlignmentLevels()
{
  int maxNumModules = pxbModuleIDs_.size();
  int maxNumLadders = pxbLadderIDs_.size() / 2; // divide by 2 since we have
                                               // HalfBarrels
  int maxNumLayers  = pxbLayerIDs_.size();

  std::ostringstream ss;
  ss << "determined following numbers for "
     << AlignableObjectId::idToString(align::TPBBarrel) << " geometry:" << "\n"
     << "   max. number of modules: " << maxNumModules                  << "\n"
     << "   max. number of ladders: " << maxNumLadders                  << "\n";

  for (size_t layer = 0; layer < pxbLaddersPerLayer_.size(); ++layer) {
    // divide by 4, because we need the ladders per quarter cylinder
    align::tpb::lpqc.push_back(pxbLaddersPerLayer_[layer] / 4);
    ss << "      ladders in layer-" << layer << ": "
       << pxbLaddersPerLayer_[layer] << "\n";
  }

  ss << "   max. number of layers:  " << maxNumLayers;
  edm::LogInfo("AlignableBuildProcess")
     << "@SUB=TrackerAlignmentLevelBuilder::buildPXBAlignmentLevels"
     << ss.str();

//  pxb_.push_back(new AlignmentLevel(align::TPBModule,     maxNumModules, false));
//  pxb_.push_back(new AlignmentLevel(align::TPBLadder,     maxNumLadders, true));
//  pxb_.push_back(new AlignmentLevel(align::TPBLayer,      maxNumLayers,  false));
//  pxb_.push_back(new AlignmentLevel(align::TPBHalfBarrel, 2,             false));
//  pxb_.push_back(new AlignmentLevel(align::TPBBarrel,     1,             false));
  pxb_->push_back(std::make_shared<AlignmentLevel>(align::TPBModule,     maxNumModules, false));
  pxb_->push_back(std::make_shared<AlignmentLevel>(align::TPBLadder,     maxNumLadders, true));
  pxb_->push_back(std::make_shared<AlignmentLevel>(align::TPBLayer,      maxNumLayers,  false));
  pxb_->push_back(std::make_shared<AlignmentLevel>(align::TPBHalfBarrel, 2,             false));
  pxb_->push_back(std::make_shared<AlignmentLevel>(align::TPBBarrel,     1,             false));
}

//_____________________________________________________________________________
void TrackerAlignmentLevelBuilder
::buildPXEAlignmentLevels()
{
  int maxNumModules = pxeModuleIDs_.size();
  int maxNumPanels  = pxePanelIDs_.size();
  int maxNumBlades  = pxeBladeIDs_.size() / 2;
  int maxNumDisks   = pxeDiskIDs_.size();
  int maxNumSides   = pxeSideIDs_.size();

  std::ostringstream ss;
  ss << "determined following numbers for "
     << AlignableObjectId::idToString(align::TPEEndcap) << " geometry:" << "\n"
     << "   max. number of modules: " << maxNumModules                  << "\n"
     << "   max. number of panels:  " << maxNumPanels                   << "\n"
     << "   max. number of blades:  " << maxNumBlades                   << "\n";

  align::tpe::bpqd = maxNumBlades / 2;

  ss << "      blades per quarter disk: " << align::tpe::bpqd << "\n"
     << "   max. number of disks:   " << maxNumDisks                    << "\n"
     << "   max. number of sides:   " << maxNumSides;
  edm::LogInfo("AlignableBuildProcess")
     << "@SUB=TrackerAlignmentLevelBuilder::buildPXEAlignmentLevels"
     << ss.str();

  pxe_->push_back(std::make_shared<AlignmentLevel>(align::TPEModule,       maxNumModules, false));
  pxe_->push_back(std::make_shared<AlignmentLevel>(align::TPEPanel,        maxNumPanels,  true));
  pxe_->push_back(std::make_shared<AlignmentLevel>(align::TPEBlade,        maxNumBlades,  true));
  pxe_->push_back(std::make_shared<AlignmentLevel>(align::TPEHalfDisk,     maxNumDisks,   false));
  pxe_->push_back(std::make_shared<AlignmentLevel>(align::TPEHalfCylinder, 2,             false));
  pxe_->push_back(std::make_shared<AlignmentLevel>(align::TPEEndcap,       maxNumSides,   false));
}

//_____________________________________________________________________________
void TrackerAlignmentLevelBuilder
::buildTIBAlignmentLevels()
{
  int maxNumModules = tibModuleIDs_.size();
  int maxNumStrings = tibStringIDs_.size();
  int maxNumLayers  = tibLayerIDs_.size();
  int maxNumSides   = tibSideIDs_.size();

  std::ostringstream ss;
  ss << "determined following numbers for "
     << AlignableObjectId::idToString(align::TIBBarrel) << " geometry:" << "\n"
     << "   max. number of modules: " << maxNumModules                  << "\n"
     << "   max. number of strings: " << maxNumStrings                  << "\n";

  for (size_t layer = 0; layer < tidStringsInnerLayer_.size(); ++layer) {
    // divide by 2, because we have HalfShells
    align::tib::sphs.push_back(tidStringsInnerLayer_[layer] / 2);
    align::tib::sphs.push_back(tidStringsOuterLayer_[layer] / 2);

    ss << "      strings in layer-" << layer << " (inside):  "
       << tidStringsInnerLayer_[layer] << "\n"
       << "      strings in layer-" << layer << " (outside): "
       << tidStringsOuterLayer_[layer] << "\n";
  }

  ss << "   max. number of layers:  " << maxNumLayers                   << "\n"
     << "   max. number of sides:   " << maxNumSides;
  edm::LogInfo("AlignableBuildProcess")
       << "@SUB=TrackerAlignmentLevelBuilder::buildTIBAlignmentLevels"
       << ss.str();

  tib_->push_back(std::make_shared<AlignmentLevel>(align::TIBModule,     maxNumModules, false));
  tib_->push_back(std::make_shared<AlignmentLevel>(align::TIBString,     maxNumStrings, true));
  tib_->push_back(std::make_shared<AlignmentLevel>(align::TIBSurface,    2, false)); // 2 surfaces per half shell
  tib_->push_back(std::make_shared<AlignmentLevel>(align::TIBHalfShell,  2, false)); // 2 half shells per layer
  tib_->push_back(std::make_shared<AlignmentLevel>(align::TIBLayer,      maxNumLayers, false));
  tib_->push_back(std::make_shared<AlignmentLevel>(align::TIBHalfBarrel, 2, false));
  tib_->push_back(std::make_shared<AlignmentLevel>(align::TIBBarrel,     1, false));
}

//_____________________________________________________________________________
void TrackerAlignmentLevelBuilder
::buildTIDAlignmentLevels()
{
  int maxNumModules = tidModuleIDs_.size();
  int maxNumRings   = tidRingIDs_.size();
  // TODO: for PhaseII geometry the method name for tidWheel changes:
  //       -> trackerTopology->tidDisk(detId);
  int maxNumWheels  = tidWheelIDs_.size();
  int maxNumSides   = tidSideIDs_.size();

  edm::LogInfo("AlignableBuildProcess")
     << "@SUB=TrackerAlignmentLevelBuilder::buildTIDAlignmentLevels"
     << "determined following numbers for "
     << AlignableObjectId::idToString(align::TIDEndcap) << " geometry:" << "\n"
     << "   max. number of modules: " << maxNumModules                  << "\n"
     << "   max. number of rings:   " << maxNumRings                    << "\n"
     << "   max. number of wheels:  " << maxNumWheels                   << "\n"
     << "   max. number of sides:   " << maxNumSides;

  tid_->push_back(std::make_shared<AlignmentLevel>(align::TIDModule, maxNumModules, false));
  tid_->push_back(std::make_shared<AlignmentLevel>(align::TIDSide,   2,             false)); // 2 sides per ring
  tid_->push_back(std::make_shared<AlignmentLevel>(align::TIDRing,   maxNumRings,   false));
  tid_->push_back(std::make_shared<AlignmentLevel>(align::TIDDisk,   maxNumWheels,  false));
  tid_->push_back(std::make_shared<AlignmentLevel>(align::TIDEndcap, 2,             false)); // 2 endcaps in TID
}

//_____________________________________________________________________________
void TrackerAlignmentLevelBuilder
::buildTOBAlignmentLevels()
{
  int maxNumModules = tobModuleIDs_.size();
  int maxNumRods    = tobRodIDs_.size();
  int maxNumSides   = tobSideIDs_.size();
  int maxNumLayers  = tobLayerIDs_.size();

  edm::LogInfo("AlignableBuildProcess")
     << "@SUB=TrackerAlignmentLevelBuilder::buildTOBAlignmentLevels"
     << "determined following numbers for "
     << AlignableObjectId::idToString(align::TOBBarrel) << " geometry:" << "\n"
     << "   max. number of modules: " << maxNumModules                  << "\n"
     << "   max. number of rods:    " << maxNumRods                     << "\n"
     << "   max. number of sides:   " << maxNumSides                    << "\n"
     << "   max. number of layers:  " << maxNumLayers;

  tob_->push_back(std::make_shared<AlignmentLevel>(align::TOBModule,     maxNumModules, false));
  tob_->push_back(std::make_shared<AlignmentLevel>(align::TOBRod,        maxNumRods,    true));
  tob_->push_back(std::make_shared<AlignmentLevel>(align::TOBLayer,      maxNumLayers,  false));
  tob_->push_back(std::make_shared<AlignmentLevel>(align::TOBHalfBarrel, maxNumSides,   false));
  tob_->push_back(std::make_shared<AlignmentLevel>(align::TOBBarrel,     1,             false));
}

//_____________________________________________________________________________
void TrackerAlignmentLevelBuilder
::buildTECAlignmentLevels()
{
  int maxNumModules = tecModuleIDs_.size();
  int maxNumRings   = tecRingIDs_.size();
  int maxNumPetals  = tecPetalIDs_.size();
  int maxNumDisks   = tecWheelIDs_.size();
  int maxNumSides   = tecSideIDs_.size();

  edm::LogInfo("AlignableBuildProcess")
     << "@SUB=TrackerAlignmentLevelBuilder::buildTECAlignmentLevels"
     << "determined following numbers for "
     << AlignableObjectId::idToString(align::TECEndcap) << " geometry:" << "\n"
     << "   max. number of modules: " << maxNumModules                  << "\n"
     << "   max. number of rings:   " << maxNumRings                    << "\n"
     << "   max. number of petals:  " << maxNumPetals                   << "\n"
     << "   max. number of wheels:  " << maxNumDisks                    << "\n"
     << "   max. number of sides:   " << maxNumSides;

  tec_->push_back(std::make_shared<AlignmentLevel>(align::TECModule, maxNumModules, false));
  tec_->push_back(std::make_shared<AlignmentLevel>(align::TECRing,   maxNumRings,   true));
  tec_->push_back(std::make_shared<AlignmentLevel>(align::TECPetal,  maxNumPetals,  true));
  tec_->push_back(std::make_shared<AlignmentLevel>(align::TECSide,   2,             false)); // 2 sides per disk
  tec_->push_back(std::make_shared<AlignmentLevel>(align::TECDisk,   maxNumDisks,   false));
  tec_->push_back(std::make_shared<AlignmentLevel>(align::TECEndcap, 2,             false));
}
