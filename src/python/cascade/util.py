import os
import sys
import random
import string
import fnmatch
import math

import gzip

import cascade

sqrt2pi = math.sqrt(2 * math.pi)

def gzipped(infile, outfile=None, rem=True):
    if not outfile:
        outfile = infile + '.gz'
    f_in = open(infile, 'rb')
    f_out = gzip.open(outfile, 'wb')
    f_out.writelines(f_in)
    f_out.close()
    f_in.close()
    if rem:
        os.remove(infile)
        
def ensureAbsence(fname):
    if fname and os.path.exists(fname): os.remove(fname)

def ensureDirPresence(fname):
    directory_name = os.path.dirname(fname)
    if not os.path.exists(directory_name):
        os.makedirs(directory_name)

def ensureDirectory(dname):
    if not os.path.exists(dname):
        os.makedirs(dname)
    
def cumsum(iterable):
    iterable= iter(iterable)
    s= iterable.next()
    yield s
    for c in iterable:
        s= s + c
        yield s

def fileSeries(prefix, ext='.nii.gz'):
    i = 0
    while True:
        fname  = '{}{:04d}{}'.format(prefix, i, ext)
        if os.path.exists(fname):
            i = i + 1
            yield fname
        else:
            break
        
def randomFilename(size=8, chars=string.ascii_letters):
    return ''.join(random.choice(chars) for _ in range(size))

def findDir(root, pattern='*'):
    breakFlag = False
    for rootDir , dirs, files in os.walk(root):
        for d in dirs:
            if fnmatch.fnmatch(d, pattern):
                yield os.path.join(rootDir, d)
                breakFlag = True
        if breakFlag: break

def findFile(root, pattern='*'):
    for rootDir , dirs, files in os.walk(root):
        for f in files:
            if fnmatch.fnmatch(f, pattern):
                yield os.path.join(rootDir, f)

def convolve(sig, kernel):
    l = len(sig)
    lk = len(kernel)
    lm = int((lk-1)/2)
    sigC = [None]*l
    for i, _ in enumerate(sig):
        minK = 0 if i > lm else lm -i
        maxK = lk if i < l - lm else l-1-(lm + i)+lk
        minS = min(i, l-lm-1)
        maxS = minS + maxK - minK
        sigL = sig[minS:maxS]
        kerL = kernel[minK:maxK]
        kerL = [k/ sum(kerL) for k in kerL]
        sigC[i] = sum([p*q for p,q in zip(sigL, kerL)])
    return sigC

def xlinspace(l, begin, end):
    for j in xrange(l):
        yield float(j)/(l-1)*(end-begin) + begin

def linspace(l, begin, end):
    return list(xlinspace(l, begin, end))

def imageHistogram(nImg, mask=None, nbins = 100):
    maskParam = ['-l', 0]
    if mask and os.path.exists(mask):
        maskParam = ['-k', mask]
    maxImage = cascade.binary_proxy.fsl_check_output('fslstats', [nImg] + maskParam + ['-R'])
    maxImage = map(float, maxImage.split())
    bins = cascade.util.linspace(nbins, 0, maxImage[1])
    hist = cascade.binary_proxy.fsl_check_output('fslstats', [nImg] + maskParam + ['-H', nbins, 0, maxImage[1]])
    hist = cascade.util.convolve(map(float, hist.split()), [0.25, 0.5, 0.25])
    return hist, bins


def _histogramStats(bins, hist, peakPercentage=0.0):
    peak = max(hist)
    hist = filter (lambda x: x[0]>=peak*peakPercentage, zip(hist,bins))
    sumweight = 0
    mean = 0
    M2 = 0
    for weight, x  in hist:
        temp = weight + sumweight
        delta = x - mean
        R = delta * weight / temp
        mean = mean + R
        M2 = M2 + sumweight * delta * R
        sumweight = temp
 
    variance_n = M2/sumweight
    variance = variance_n * len(hist)/(len(hist) - 1)
    return mean, variance

def norm(x, m, s):
    return math.exp(-0.5 * ((x - m) / s) ** 2) / (s * sqrt2pi)

def _normFactor(percentage):
    x = linspace(100,-6,6)
    print 'Calculating norm factor for', percentage
    return _histogramStats(x, map(lambda a: norm(a, 0, 1), x), percentage)[1]
_nf = {}
def normFactor(percentage):
    if not _nf.has_key(percentage):
        _nf[percentage] = _normFactor(percentage)
    return _nf[percentage]

def histogramStats(bins, hist, peakPercentage=0.0):
    mean, variance = _histogramStats(bins, hist, peakPercentage)
    nf=normFactor(peakPercentage)
    return mean, variance/nf

def histExtent(hist, bins, ext=2, peakPercentage=0.5):
    statH = histogramStats(bins, hist, peakPercentage)
    return statH[0] - ext * math.sqrt(statH[1]),statH[0] + ext * math.sqrt(statH[1]) 


def normIntersection(m1, s1, m2, s2):
    a = 1 / (2 * math.pow(s1, 2)) - 1 / (2 * math.pow(s2, 2))
    b = m2 / (math.pow(s2, 2)) - m1 / (math.pow(s1, 2))
    c = math.pow(m1, 2) / (2 * math.pow(s1, 2)) - math.pow(m2, 2) / (2 * math.pow(s2, 2)) + math.log(s1) - math.log(s2)
    
    x1 = (-b - math.sqrt(math.pow(b, 2) - 4 * a * c)) / (2 * a)
    x2 = (-b + math.sqrt(math.pow(b, 2) - 4 * a * c)) / (2 * a)
    return min(x1, x2), max(x1, x2)
    
def updateGMM(dens, bins, ms, ss):
    newW = [0] * len(ms)
    newM = ms
    newS = ss
    xs = range(len(dens))
    Apr = [ [norm(x, m, s) for x in bins] for m, s in zip(ms, ss)]
    Apr = zip(*Apr)
    for i in range(len(ms)):
        prSum = 0
        prM1 = 0
        prM2 = 0
        for x, w, modelPrs in zip(bins, dens, Apr):
            pr = w * modelPrs[i] / sum(modelPrs)
            prSum += pr
            prM1 += pr * x
            prM2 += pr * x * x
        newW[i] = prSum
        newM[i] = prM1 / prSum
        newS[i] = math.sqrt(prM2 / prSum - (newM[i] * newM[i])) 
    
    return newW, newM, newS

def gmm(hist, bins, n=3, iteration=1000):
    total = sum(hist)
    dens = [h / total for h in hist]
    W = [1.0 / n] * n
    S = [math.sqrt((bins[-1] - bins[0]) / n)] * n
    M = [bins[int(i)] for i in linspace(n + 2, 1, len(hist))[1:-1]]
    
    for i in range(iteration):
       W, M, S = updateGMM(dens, bins, M, S)
    return W, M, S