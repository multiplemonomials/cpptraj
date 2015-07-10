#include "Trajout_Single.h"
#include "CpptrajStdio.h"
#include "StringRoutines.h" // NumberFilename() TODO put in FileName class

// DESTRUCTOR
Trajout_Single::~Trajout_Single() {
  EndTraj();
  if (trajio_!=0) delete trajio_;
}

// Trajout_Single::InitTrajWrite()
/** Initialize output trajectory with appropriate TrajectoryIO class and 
  * process arguments.
  */
int Trajout_Single::InitTrajWrite(std::string const& tnameIn, ArgList const& argIn,
                                  TrajectoryFile::TrajFormatType writeFormatIn)
{
  // Require a filename
  if (tnameIn.empty()) {
    mprinterr("Internal Error: InitTrajWrite: No filename given.\n");
    return 1;
  }
  return InitTrajout(tnameIn, argIn, writeFormatIn);
}

// Trajout_Single::PrepareStdoutTrajWrite()
/** Initialize and set up output trajectory for STDOUT write. */
int Trajout_Single::PrepareStdoutTrajWrite(ArgList const& argIn, Topology *tparmIn,
                                 TrajectoryFile::TrajFormatType writeFormatIn)
{
  if (InitTrajout("", argIn, writeFormatIn)) return 1;
  if (SetupTrajWrite(tparmIn)) return 1;
  return 0;
}

// Trajout_Single::InitEnsembleTrajWrite()
int Trajout_Single::InitEnsembleTrajWrite(std::string const& tnameIn, ArgList const& argIn,
                                          Topology* tparmIn, TrajectoryFile::TrajFormatType fmtIn,
                                          int ensembleNum)
{
  FileName tempName;
  tempName.SetFileName( tnameIn );
  TrajectoryFile::TrajFormatType fmt = fmtIn;
  if (fmt == TrajectoryFile::UNKNOWN_TRAJ)
    fmt = TrajectoryFile::GetTypeFromExtension( tempName.Ext() );
  int err = 0;
  if (ensembleNum > -1)
    err = InitTrajWrite( NumberFilename(tnameIn, ensembleNum), argIn, fmt );
  else
    err = InitTrajWrite( tnameIn,                              argIn, fmt );
  if (err != 0) return 1;
  if (SetupTrajWrite(tparmIn)) return 1;
  return 0;
}

// Trajout_Single::InitTrajout()
int Trajout_Single::InitTrajout(std::string const& tnameIn, ArgList const& argIn,
                                TrajectoryFile::TrajFormatType writeFormatIn)
{
  ArgList trajout_args = argIn;
  // Process common args
  if (traj_.CommonTrajoutSetup(tnameIn, trajout_args, writeFormatIn))
    return 1;
  if (trajio_ != 0) delete trajio_;
  // If appending, file must exist and must match the current format.
  // TODO Do not pass in writeformat directly to be changed.
  if (traj_.Append()) {
    if (traj_.CheckAppendFormat( traj_.Filename().Full(), traj_.WriteFormat() ))
      traj_.SetAppend( false );
  }
  // Set up for the specified format.
  trajio_ = TrajectoryFile::AllocTrajIO( traj_.WriteFormat() );
  if (trajio_ == 0) return 1;
  mprintf("\tWriting '%s' as %s\n", traj_.Filename().full(),
          TrajectoryFile::FormatString(traj_.WriteFormat()));
  trajio_->SetDebug( debug_ );
  // Set specified title - will not set if empty 
  trajio_->SetTitle( traj_.Title() );
  // Process any write arguments specific to certain formats not related
  // to parm file. Options related to parm file are handled in SetupTrajWrite 
  if (trajio_->processWriteArgs(trajout_args)) {
    mprinterr("Error: trajout %s: Could not process arguments.\n", traj_.Filename().full());
    return 1;
  }
  // Write is set up for topology in SetupTrajWrite 
  return 0;
}

// Trajout_Single::EndTraj()
void Trajout_Single::EndTraj() {
  //if (TrajIsOpen()) { // FIXME: Necessary?
    trajio_->closeTraj();
  //  SetTrajIsOpen(false);
  //}
}

/** Perform any topology-related setup for this trajectory */
// TODO pass in nframes and coordinateinfo. Should former be part of latter?
int Trajout_Single::SetupTrajWrite(Topology* tparmIn) {
  // Set up topology and coordinate info.
  if (traj_.SetupCoordInfo(tparmIn, tparmIn->Nframes(), tparmIn->ParmCoordInfo()))
    return 1;
  if (debug_ > 0)
    rprintf("\tSetting up %s for WRITE, topology '%s' (%i atoms).\n",
            traj_.Filename().base(), tparmIn->c_str(), tparmIn->Natom());
  // Set up TrajectoryIO
  if (trajio_->setupTrajout(traj_.Filename().Full(), traj_.Parm(), traj_.CoordInfo(),
                            traj_.NframesToWrite(), traj_.Append()))
    return 1;
  if (debug_ > 0)
    Frame::PrintCoordInfo(traj_.Filename().base(), traj_.Parm()->c_str(), trajio_->CoordInfo());
  // First frame setup
  //if (!TrajIsOpen()) { //}
  return 0;
}

// Trajout_Single::WriteSingle()
/** Write given frame if trajectory is open (initialzed and set-up).
  */
int Trajout_Single::WriteSingle(int set, Frame const& FrameOut) {
  // Check that set should be written
  if (traj_.CheckFrameRange(set)) return 0;
  // Write
  //fprintf(stdout,"DEBUG: %20s: Writing %i\n",trajName,set);
  if (trajio_->writeFrame(set, FrameOut)) return 1;
  return 0;
}

// Trajout_Single::PrintInfo()
void Trajout_Single::PrintInfo(int showExtended) const {
  mprintf("  '%s' ", traj_.Filename().base());
  trajio_->Info();
  traj_.CommonInfo();
}
