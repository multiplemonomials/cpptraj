#!/bin/bash

. ../MasterTest.sh

CleanFiles cluster.in cnumvtime.dat avg.summary.dat summary.dat \
           CpptrajPairDist clusterinfo.txt cluster.c? rep.*.crd \
           single lifetime.dat Avg.c?.rst7

# Test 1
CheckNetcdf
cat > cluster.in <<EOF
parm ../tz2.parm7
trajin ../tz2.nc
createcrd CRD1
run
runanalysis cluster crdset CRD1 C1 :2-10 clusters 3 epsilon 4.0 \
            out cnumvtime.dat summary avg.summary.dat nofit \
            clusterout cluster info clusterinfo.txt \
            singlerepout single repout rep repframe lifetime \
            avgout Avg avgfmt restart
write lifetime.dat C1[Lifetime]
EOF
INPUT="-i cluster.in"
RunCpptraj "Cluster command test, coordinate writes."
DoTest clusterinfo.txt.save clusterinfo.txt
DoTest cluster.c0.save cluster.c0
DoTest rep.1.crd.save rep.c8.1.crd
DoTest single.save single
DoTest avg.summary.dat.save avg.summary.dat
DoTest lifetime.dat.save lifetime.dat
DoTest Avg.c0.rst7.save Avg.c0.rst7 
CheckTest

EndTest

exit 0
