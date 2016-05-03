// -*- C++ -*-
//
// Package:    Alignment/TrackerAlignment
// Class:      CreateIdealTkAlRecords
//
/**\class CreateIdealTkAlRecords CreateIdealTkAlRecords.cc Alignment/TrackerAlignment/plugins/CreateIdealTkAlRecords.cc

 Description: Plugin to create ideal tracker alignment records.

 Implementation:
     The plugin takes the geometry stored in the global tag and transfers this
     information to the format needed in the TrackerAlignmentRcd. The APEs are
     set to zero for all det IDs of the tracker geometry and put into an
     TrackerAlignmentErrorExtendedRcd.

*/
//
// Original Author:  Gregor Mittag
//         Created:  Tue, 26 Apr 2016 09:45:13 GMT
//
//


// system include files
#include <memory>
#include <iostream>

// user include files
#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/one/EDAnalyzer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/Framework/interface/ESHandle.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ServiceRegistry/interface/Service.h"

#include "CondCore/DBOutputService/interface/PoolDBOutputService.h"

#include "CondFormats/Alignment/interface/Alignments.h"
#include "CondFormats/Alignment/interface/AlignTransform.h"
#include "CondFormats/Alignment/interface/AlignmentErrorsExtended.h"
#include "CondFormats/Alignment/interface/AlignTransformError.h"
#include "CondFormats/Alignment/interface/AlignTransformErrorExtended.h"
#include "CondFormats/AlignmentRecord/interface/TrackerAlignmentRcd.h"
#include "CondFormats/AlignmentRecord/interface/TrackerAlignmentErrorExtendedRcd.h"
#include "CondFormats/GeometryObjects/interface/PTrackerParameters.h"

#include "DataFormats/TrackerCommon/interface/TrackerTopology.h"

#include "Geometry/TrackerGeometryBuilder/interface/TrackerGeometry.h"
#include "Geometry/TrackerGeometryBuilder/interface/TrackerGeomBuilderFromGeometricDet.h"
#include "Geometry/Records/interface/IdealGeometryRecord.h"
#include "Geometry/Records/interface/TrackerTopologyRcd.h"
#include "Geometry/Records/interface/PTrackerParametersRcd.h"

#include "CLHEP/Vector/RotationInterfaces.h"

//
// class declaration
//

class CreateIdealTkAlRecords : public edm::one::EDAnalyzer<>  {
public:
  explicit CreateIdealTkAlRecords(const edm::ParameterSet&);
  ~CreateIdealTkAlRecords();

  static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);
  static std::string toString(const GeomDetEnumerators::SubDetector&);

private:
  virtual void analyze(const edm::Event&, const edm::EventSetup&) override;
  void clearAlignmentInfos();
  std::unique_ptr<TrackerGeometry> retrieveGeometry(const edm::EventSetup&);
  void addAlignmentInfo(const GeomDet&);
  void alignToGT(const edm::EventSetup&);
  void writeToDB();

  // ----------member data ---------------------------
  const bool alignToGlobalTag_;
  const bool createReferenceRcd_;
  bool firstEvent_;
  Alignments alignments_;
  AlignmentErrorsExtended alignmentErrors_;
  std::vector<uint32_t> rawIDs_;
};


//
// constructors and destructor
//
CreateIdealTkAlRecords::CreateIdealTkAlRecords(const edm::ParameterSet& iConfig) :
  alignToGlobalTag_(iConfig.getUntrackedParameter<bool>("alignToGlobalTag")),
  createReferenceRcd_(iConfig.getUntrackedParameter<bool>("createReferenceRcd")),
  firstEvent_(true)
{
}


CreateIdealTkAlRecords::~CreateIdealTkAlRecords()
{
}


//
// member functions
//

// ------------ method called for each event  ------------
void
CreateIdealTkAlRecords::analyze(const edm::Event&, const edm::EventSetup& iSetup)
{
  if (firstEvent_) {
    clearAlignmentInfos();
    const auto tracker = retrieveGeometry(iSetup);

    auto dets = tracker->dets();
    std::sort(dets.begin(), dets.end(),
              [](const auto& a, const auto& b) {
                return a->geographicalId().rawId() < b->geographicalId().rawId();});

    for (const auto& det: dets) addAlignmentInfo(*det);
    if (alignToGlobalTag_ && !createReferenceRcd_) alignToGT(iSetup);
    writeToDB();
    firstEvent_ = false;
  }
}


std::string
CreateIdealTkAlRecords::toString(const GeomDetEnumerators::SubDetector& sub)
{
  switch (sub) {
  case GeomDetEnumerators::PixelBarrel: return "PixelBarrel";
  case GeomDetEnumerators::PixelEndcap: return "PixelEndcap";
  case GeomDetEnumerators::TIB:         return "TIB";
  case GeomDetEnumerators::TOB:         return "TOB";
  case GeomDetEnumerators::TID:         return "TID";
  case GeomDetEnumerators::TEC:         return "TEC";
  case GeomDetEnumerators::CSC:         return "CSC";
  case GeomDetEnumerators::DT:          return "DT";
  case GeomDetEnumerators::RPCBarrel:   return "RPCBarrel";
  case GeomDetEnumerators::RPCEndcap:   return "RPCEndcap";
  case GeomDetEnumerators::GEM:         return "GEM";
  case GeomDetEnumerators::ME0:         return "ME0";
  case GeomDetEnumerators::P2OTB:       return "P2OTB";
  case GeomDetEnumerators::P2OTEC:      return "P2OTEC";
  case GeomDetEnumerators::P1PXB:       return "P1PXB";
  case GeomDetEnumerators::P1PXEC:      return "P1PXEC";
  case GeomDetEnumerators::P2PXEC:      return "P2PXEC";
  case GeomDetEnumerators::invalidDet:  return "invalidDet";
  default:
    throw cms::Exception("UnknownSubdetector");
  }
}


void
CreateIdealTkAlRecords::clearAlignmentInfos()
{
  alignments_.clear();
  alignmentErrors_.clear();
  rawIDs_.clear();
}


std::unique_ptr<TrackerGeometry>
CreateIdealTkAlRecords::retrieveGeometry(const edm::EventSetup& iSetup)
{
  edm::ESHandle<GeometricDet> geometricDet;
  iSetup.get<IdealGeometryRecord>().get(geometricDet);

  edm::ESHandle<PTrackerParameters> ptp;
  iSetup.get<PTrackerParametersRcd>().get(ptp);

  edm::ESHandle<TrackerTopology> tTopoHandle;
  iSetup.get<TrackerTopologyRcd>().get(tTopoHandle);
  const auto* const tTopo = tTopoHandle.product();

  TrackerGeomBuilderFromGeometricDet trackerBuilder;

  return std::unique_ptr<TrackerGeometry> {
    trackerBuilder.build(&(*geometricDet), *ptp, tTopo )};
}


void
CreateIdealTkAlRecords::addAlignmentInfo(const GeomDet& det)
{
  const auto subDetector = toString(det.subDetector());
  const auto& detId = det.geographicalId().rawId();
  const auto& pos = det.position();
  const auto& rot = det.rotation();
  rawIDs_.push_back(detId);

  // TrackerAlignmentRcd entry
  if (createReferenceRcd_) {
    alignments_.m_align.emplace_back(AlignTransform(AlignTransform::Translation(),
						    AlignTransform::Rotation(),
						    detId));
  } else {
    const AlignTransform::Translation translation(pos.x(), pos.y(), pos.z());
    const AlignTransform::Rotation rotation(
        CLHEP::HepRep3x3(rot.xx(),rot.xy(),rot.xz(),
			 rot.yx(),rot.yy(),rot.yz(),
			 rot.zx(),rot.zy(),rot.zz()));
    const auto& eulerAngles = rotation.eulerAngles();
    LogDebug("Alignment")
      << "============================================================\n"
      << "subdetector: " << subDetector << "\n"
      << "detId:       " << detId << "\n"
      << "------------------------------------------------------------\n"
      << "     x: " << pos.x() << "\n"
      << "     y: " << pos.y() << "\n"
      << "     z: " << pos.z() << "\n"
      << "   phi: " << eulerAngles.phi() << "\n"
      << " theta: " << eulerAngles.theta() << "\n"
      << "   psi: " << eulerAngles.psi() << "\n"
      << "============================================================\n";
    alignments_.m_align.emplace_back(AlignTransform(translation, rotation, detId));
  }

  // TrackerAlignmentErrorExtendedRcd entry
  const AlignTransformError::SymMatrix zeroAPEs(6, 0);
  alignmentErrors_.m_alignError.emplace_back(AlignTransformErrorExtended(zeroAPEs, detId));
}


void
CreateIdealTkAlRecords::alignToGT(const edm::EventSetup& iSetup)
{
  edm::ESHandle<Alignments> alignments;
  iSetup.get<TrackerAlignmentRcd>().get(alignments);
  edm::ESHandle<AlignmentErrorsExtended> alignmentErrors;
  iSetup.get<TrackerAlignmentErrorExtendedRcd>().get(alignmentErrors);
  if (alignments->m_align.size() != alignmentErrors->m_alignError.size())
    throw cms::Exception("GeometryMismatch")
      << "Size mismatch between alignments (size=" << alignments->m_align.size()
      << ") and alignment errors (size=" << alignmentErrors->m_alignError.size()
      << ")";

  auto itAlignErr = alignmentErrors->m_alignError.cbegin();
  for (auto itAlign = alignments->m_align.cbegin();
       itAlign != alignments->m_align.cend();
       ++itAlign, ++itAlignErr) {
    const auto id = itAlign->rawId();
    auto found = std::find(rawIDs_.cbegin(), rawIDs_.cend(), id);
    if (found != rawIDs_.end()) {
      if (id != itAlignErr->rawId())
        throw cms::Exception("GeometryMismatch")
          << "DetId mismatch between alignments (rawId=" << id
          << ") and alignment errors (rawId=" << itAlignErr->rawId() << ")";

      const auto index = std::distance(rawIDs_.cbegin(), found);

      if (alignments_.m_align[index].rawId()
          != alignmentErrors_.m_alignError[index].rawId())
        throw cms::Exception("GeometryMismatch")
          << "DetId mismatch between alignments (rawId="
          << alignments_.m_align[index].rawId()
          << ") and alignment errors (rawId="
          << alignmentErrors_.m_alignError[index].rawId() << ")";

      alignments_.m_align[index] = *itAlign;
      alignmentErrors_.m_alignError[index] = *itAlignErr;
    }
  }
}


void
CreateIdealTkAlRecords::writeToDB()
{
  const auto& since = cond::timeTypeSpecs[cond::runnumber].beginValue;

  edm::Service<cond::service::PoolDBOutputService> poolDb;
  if (!poolDb.isAvailable()) {
    throw cms::Exception("NotAvailable") << "PoolDBOutputService not available";
  }

  edm::LogInfo("Alignment")
    << "Writing ideal tracker-alignment records.";
  poolDb->writeOne<Alignments>(&alignments_, since, "TrackerAlignmentRcd");
  poolDb->writeOne<AlignmentErrorsExtended>(&alignmentErrors_, since,
                                            "TrackerAlignmentErrorExtendedRcd");
}


// ------------ method fills 'descriptions' with the allowed parameters for the module  ------------
void
CreateIdealTkAlRecords::fillDescriptions(edm::ConfigurationDescriptions& descriptions)
{
  edm::ParameterSetDescription desc;
  desc.setComment("Creates ideal TrackerAlignmentRcd and TrackerAlignmentErrorExtendedRcd "
                  "from the loaded tracker geometry. "
                  "PoolDBOutputService must be set up for these records.");
  desc.addUntracked<bool>("alignToGlobalTag", false);
  desc.addUntracked<bool>("createReferenceRcd", false);
  descriptions.add("createIdealTkAlRecords", desc);
}


//define this as a plug-in
DEFINE_FWK_MODULE(CreateIdealTkAlRecords);
