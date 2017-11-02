# @Author: Rostyslav Shevchenko <shevchen>
# @Date:   01-Jan-1970
# @Email:  shevchenko.rostislav@gmail.com
# @Filename: Mpslibclass.py
# @Last modified by:   shevchen
# @Last modified time: 01-Nov-2017



# This Jobdatabas-Class interacts with the mps.db file.
# It's member-variables are often called in the mps_... scripts.
#
# Meaning of the database variables:
#
# (1) Header
#       header          - version information
#       batchScript     - base script for serial job
#       cfgTemplate     - template for cfg file
#       infiList        - list of input files to be serialized
#       classInf        - batch class information
#                          (might contain two ':'-separated)
#       addFiles        - job name for submission
#       driver          - specifies whether merge job is foreseen
#       nJobs           - number of serial jobs (not including merge job)
#       mergeScript     - base script for merge job
#       mssDir          - directory for mass storage (e.g. Castor)
#       updateTime      - time of last update (seconds since 1970)
#       updateTimeHuman - time of last update (human readable)
#       elapsedTime     - seconds since last update
#       mssDirPool      - pool for $mssDir (e.g. cmscaf/cmscafuser)
#       pedeMem         - Memory allocated for pede
#       spare1
#       spare2
#       spare3

# (2) Job-level variables/lists
#       JOBNUMBER   - ADDED, selfexplanatory
#       JOBDIR      - name of job directory (not full path)
#       JOBSTATUS   - status of job
#       JOBRUNTIME  - present CPU time of job
#       JOBNEVT     - number of events processed by job
#       JOBHOST     - presently used to store remark
#       JOBINCR     - CPU increment since last check
#       JOBREMARK   - comment
#       JOBSP1      - spare
#       JOBSP2      - possible weight for pede
#       JOBSP3      - possible name as given to mps_setup.pl -N <name> ...
#       JOBID       - ID of the LSF/HTCondor job

import copy

import datetime
import time
import os
import sys

__version__     = "0.0.1"

class Job:

    def __init__(self, *args = None):
        '''Default constructor

        '''
        if args != None and len(args) == 13:
            self.set(args[0],args[1],args[2],args[3],args[4],args[5],
            args[6],args[7],args[8],args[9],args[10],args[11],args[12])

    def copy(self):
        '''Copy constructor.

        '''
        return copy.deepcopy(self)


    def set(self,JOBNUMBER,JOBDIR,JOBID,JOBSTATUS,JOBNTRY,
    JOBRUNTIME, JOBNEVT, JOBHOST, JOBINCR, JOBREMARK,
    JOBSP1,JOBSP2,JOBSP3):
        '''Setter.

        '''
        self.JOBNUMBER = JOBNUMBER
        self.JOBDIR    = JOBDIR
        self.JOBID     = JOBID
        self.JOBSTATUS = JOBSTATUS
        self.JOBNTRY   = JOBNTRY
        self.JOBRUNTIME= JOBRUNTIME
        self.JOBNEVT   = JOBNEVT
        self.JOBHOST   = JOBHOST
        self.JOBINCR   = JOBINCR
        self.JOBREMARK = JOBREMARK
        self.JOBSP1    = JOBSP1
        self.JOBSP2    = JOBSP2
        self.JOBSP3    = JOBSP3


    def printJob(self):
        '''print interesting Job-level lists.

        '''
        merge = self.isMergeJob()
        print_str = '%6s  %9s  %6s  %3d  %5d  %8d  %8s  %5s  %s' %(
            self.JOBDIR, self.JOBID, self.JOBSTATUS,
            self.JOBNTRY, self.JOBRUNTIME, self.JOBNEVT, self.JOBHOST,
            self.JOBSP2, self.JOBSP3)
        if not merge:
            print_str = '%03d  ' % self.JOBNUMBER + print_str
        else:
            print_str = 'MMM  ' + print_str

        print print_str


    def __str__(self):
        '''SQL stile string

        '''
        return '%s:%s:%s:%s:%s:%s:%s:%s:%s:%s:%s:%s\n' % (
    self.JOBDIR, self.JOBID, self.JOBSTATUS,
    self.JOBNTRY, self.JOBRUNTIME, self.JOBNEVT, self.JOBHOST,
    self.JOBINCR, self.JOBREMARK, self.JOBSP1, self.JOBSP2, self.JOBSP3
    )

    def isMergeJob(self):
        '''Check whether job is the merge job.

        '''
        if self.JOBDIR.startswith('jobm'):
            return True
        else:
            return False


#-------------------------------------------------------------------------------
class jobdatabase:

    def read_db(self, db_file_name = "mps.db"):
        self.fileName = db_file_name
        DBFILE = self._open_db_file(db_file_name)
        # get general info from the DB file
        self.readDBheader(DBFILE)
        # read the db line-by-line
        self.readDBContent(DBFILE)
        DBFILE.close()

    def _open_db_file(self,db_file_name):
        try:
            my_file = open(db_file_name,'r')
        except IOError as e:
            if e.args != (2, 'No such file or directory'):
                raise
            else:
                msg = "No " + db_file_name + " found. Make sure you are in "
                "a campaign directory and that the campaign is set up."
            raise IOError(msg)
        return my_file

    def readDBheader(self,db_file):
        '''Read infolines at the top of SQL .db file.

        Lines are read to the class member vars
        '''
        self.header          = db_file.readline().strip()
        self.batchScript     = db_file.readline().rstrip('\n')
        self.cfgTemplate     = db_file.readline().rstrip('\n')
        self.infiList        = db_file.readline().rstrip('\n')
        self.classInf        = db_file.readline().rstrip('\n')
        self.addFiles        = db_file.readline().rstrip('\n')
        self.driver          = db_file.readline().rstrip('\n')
        self.mergeScript     = db_file.readline().rstrip('\n')
        self.mssDir          = db_file.readline().rstrip('\n')
        self.updateTime      = int(db_file.readline())
        self.updateTimeHuman = db_file.readline().rstrip('\n')
        self.elapsedTime     = int(db_file.readline())
        self.mssDirPool      = db_file.readline().rstrip('\n')
        self.pedeMem         = int(db_file.readline())
        self.spare1          = db_file.readline().rstrip('\n')
        self.spare2          = db_file.readline().rstrip('\n')
        self.spare3          = db_file.readline().rstrip('\n')

    def readDBContent(self,db_file):
        '''Read a content of the database file and store.

        '''
        self.nJobs = 0
        for line in db_file:
            line = line.rstrip('\n')
            line_parts = line.split(':')
            #set the Job instance
            j.set(int(parts[0]),parts[1].strip(),parts[2],parts[3].strip(),
            int(parts[4]),int(part[5]),int(parts[6]),parts[7].strip(),
            int(parts[8]),parts[9].strip(),parts[10].strip(),parts[11].strip(),
            parts[12].strip())
            #increment number of jobs
            self._incrementNumberOfMilleJobs(j.isMergeJob())
            # append the job
            self.Jobs.append(j)

    def _incrementNumberOfMilleJobs(self,merge):
        if not merge:
            self.nJobs += 1


    def totalEvents(self):
        '''Total Number of Events in the .db.

        '''
        return sum(job.JOBNEVT for job in self.Jobs if not job.isMergeJob())


    def print_memdb(self):
        '''Print information about the database.

        '''
        #print metainfo
        print "\n=== mps database printout ===\n"
        print self._header
        print 'Script:\t\t',    self._batchScript
        print 'cfg:\t\t',       self._cfgTemplate
        print 'files:\t\t',     self._infiList
        print 'class:\t\t',     self._classInf
        print 'name:\t\t',      self._addFiles
        print 'driver:\t\t',    self._driver
        print 'mergeScript:\t', self._mergeScript
        print 'mssDir:\t\t',    self._mssDir
        print 'updateTime:\t',  self._updateTimeHuman
        print 'elapsed:\t',     self._elapsedTime
        print 'mssDirPool:\t',  self._mssDirPool
        print 'pedeMem:\t',     self._pedeMem, '\n'

        #print interesting Job-level lists
        print '###     dir      jobid    stat  try  rtime      nevt  remark   '
        'weight  name'
        print "---------------------------------------------------------------'
        '---------------"
        for job in self.Jobs:
            job.printJob()

        #print summed info
        totalEvents = 0
        totalCpu = 0
        for job in self.Jobs:
            if not job.isMergeJob():
                totalEvents += job.JOBNEVT
                totalCpu += job.JOBRUNTIME

        meanCpuPerEvent = 0.
        if totalEvents > 0:
            meanCpuPerEvent = float(totalCpu)/totalEvents
        print "------------------------------------------------------------------------------"
        print "\t\t\t\t\tEvent total:\t",       totalEvents
        print "\t\t\t\t\tCPU total:\t",         totalCpu,               's'
        print "\t\t\t\t\tMean CPU/event:\t",meanCpuPerEvent,'s'


    def _setNewDBheader(self,new_db):

        new_db.header = "mps database schema 3.2"
        new_db.currentTime = int(time.time())
        new_db.elapsedTime = 0;
        if self.updateTime != 0:
            new_db.elapsedTime = self.currentTime - self.updateTime
            new_db.updateTime = self.currentTime
            new_db.updateTimeHuman = str(datetime.datetime.today())
            new_db.spare1 = "-- unused --"
            new_db.spare2 = "-- unused --"
            new_db.spare3 = "-- unused --"


    def writeHeaderData(self,db_file):
        headData = [self.header, self.batchScript, self.cfgTemplate,
                    self.infiList, self.classInf, self.addFiles,
                    self.driver, self.mergeScript, self.mssDir,
                    self.updateTime, self.updateTimeHuman, self.elapsedTime,
                    self.mssDirPool, self.pedeMem, self.spare1, self.spare2,
                    self.spare3 ]
        for item in headData:
            db_file.write("%s\n" % item)


    def backup(self):
        '''if file.db already exists, backup as file.db~

        '''
        os.system('[[ -a ' + self.fileName + ' ]] && cp -p '+ self.fileName + ' ' + self.fileName + '~')


    def write_db(self, fileName = 'mps.db'):
        '''Writes a new mps.db file from the members.

        Old one is replaced
        '''
        self._setNewDBheader(new_db)
        new_db.backup()

        #new file
        DBFILE = open(fileName,'w')
        new_db.writeHeaderData(DBFILE)
        #write jobinfo
        for i in xrange(len(self.Jobs)):
            DBFILE.write('%03d:%s\n' % (i+1,self.Jobs[i]))
        DBFILE.close()

    def get_class(self, argument=''):
        '''Returns job class as stored in db.

        one and only argument may be "mille" or "pede" for mille or pede jobs
        '''
        CLASSES = self.classInf.split(':')
        if len(CLASSES)<1 or len(CLASSES)>2:
            print '\nget_class():\n  class must be of the form \'class\' or \'classMille:classPede\', but is \'%s\'!\n\n', classInf
            sys.exit(1)
        elif argument == 'mille':
            return CLASSES[0]
        elif argument == 'pede':
            if len(CLASSES) == 1:
                return CLASSES[0]
            elif len(CLASSES) == 2:
                return CLASSES[1]
        else:
            print '\nget_class():\n  Know class only for \'mille\' or \'pede\', not %s!\n\n' %argument
            sys.exit(1)


    def pop(self):
        '''Method to remove the last Job from the jobdatabase

        '''
        return self.Jobs.pop()


    def append(self, job):
        '''Append the Job to the jobdatabase

        '''
        self.Jobs.append(job)


    # Work around for the OLD global variables #
    @property
    def JOBNUMBER(self):
        return [job.JOBNUMBER for job in self.Jobs]
        # TODO!!!
        # what to do with appends??
        # return pseudo_list(self,"JOBNUMBER") ->
        # declare a method: def append(self,value):
        # this method will


'''
    JOBNUMBER, JOBDIR, JOBID, JOBSTATUS, JOBNTRY, JOBRUNTIME, JOBNEVT, JOBHOST, JOBINCR, \
    JOBREMARK, JOBSP1, JOBSP2, JOBSP3 = ([] for i in range(13))

    header, batchScript, cfgTemplate, infiList, classInf, addFiles, driver, mergeScript, \
    mssDir, updateTimeHuman, mssDirPool, spare1, spare2, spare3 = ('' for i in range(14))

    updateTime, elapsedTime, pedeMem , nJobs = -1, -1, -1, -1
'''
