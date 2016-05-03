import FWCore.ParameterSet.Config as cms
process = cms.Process("Alignment")

process.load("Configuration.StandardSequences.MagneticField_cff") # B-field map
process.load("Configuration.Geometry.GeometryRecoDB_cff") # Ideal geometry and interface
process.load("Configuration.StandardSequences.FrontierConditions_GlobalTag_cff") # Global tag
from Configuration.AlCa.GlobalTag import GlobalTag
process.load("Alignment.TrackerAlignment.createIdealTkAlRecords_cfi")

################################################################################
# parameters to configure:                                                     #
process.GlobalTag = GlobalTag(process.GlobalTag, "auto:phase1_2017_design")    #
                                                                               #
process.createIdealTkAlRecords.alignToGlobalTag   = True                       #
################################################################################


usedGlobalTag = process.GlobalTag.globaltag._value
print "Using Global Tag:", usedGlobalTag

from CondCore.CondDB.CondDB_cfi import *
process.PoolDBOutputService = cms.Service("PoolDBOutputService",
    CondDB,
    timetype = cms.untracked.string("runnumber"),
    toPut = cms.VPSet(
        cms.PSet(
            record = cms.string("TrackerAlignmentRcd"),
            tag = cms.string("Alignments")
        ),
        cms.PSet(
            record = cms.string("TrackerAlignmentErrorExtendedRcd"),
            tag = cms.string("AlignmentErrorsExtended")
        ),
    )
)
process.PoolDBOutputService.connect = \
    ("sqlite_file:ideal_tracker_alignment_"+
     usedGlobalTag+("_reference.db"
                    if process.createIdealTkAlRecords.createReferenceRcd
                    else ".db"))

process.maxEvents = cms.untracked.PSet(input = cms.untracked.int32(1))
process.source = cms.Source("EmptySource")
process.p = cms.Path(process.createIdealTkAlRecords)
