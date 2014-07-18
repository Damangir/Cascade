import os
import math

import cascade

nbins = 100
t1 = os.path.join("/Users/soheil/Projects/MS_lesionload_timepointC/Test/FLAIR.nii.gz")


def test(img):
    lT, hT = map(float, cascade.binary_proxy.fsl_check_output('fslstats', [img, '-l', 0, '-r']).split())
    bins = cascade.util.linspace(nbins, lT, hT)    
    hist = map(float, cascade.binary_proxy.fsl_check_output('fslstats', [img, '-l', lT, '-u', hT , '-H', nbins, lT, hT]).split())
    W, M, S = cascade.util.gmm(hist, bins, 3)
    csfThr = cascade.util.normIntersection(M[0], S[0], M[1], S[1])[0]
    
if __name__ == '__main__':
    test(t1)
    
