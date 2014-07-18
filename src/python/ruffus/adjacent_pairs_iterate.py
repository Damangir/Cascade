#!/usr/bin/env python
################################################################################
#
#   adjacent_pairs_iterate.py
#
#
#   Copyright (c) 2007 Leo Goodstadt
#   
#   Permission is hereby granted, free of charge, to any person obtaining a copy
#   of this software and associated documentation files (the "Software"), to deal
#   in the Software without restriction, including without limitation the rights
#   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
#   copies of the Software, and to permit persons to whom the Software is
#   furnished to do so, subject to the following conditions:
#   
#   The above copyright notice and this permission notice shall be included in
#   all copies or substantial portions of the Software.
#   
#   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
#   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
#   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
#   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
#   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
#   THE SOFTWARE.
#################################################################################


from itertools import izip
def adjacent_pairs_iterate (array, reverse = False):
    """
    returns pairs of iterators to successive positions
    """
    if len(array) < 2:
        return
       
    # get iterators to successive positions
    if reverse:
        curr = reversed(array)
        next = reversed(array)
    else:
        curr = iter(array)
        next = iter(array)
    next.next() 
    for i, j in izip(curr, next):
        yield i, j    
    

def unit_test():
    numbers = range(10)
    print "Forward"
    for i, j in adjacent_pairs_iterate(numbers):
        print i, j
    print "Reversed"
    for i, j in adjacent_pairs_iterate(numbers, reverse=True):
        print i, j
    print numbers
        
if __name__ == '__main__':
    unit_test()




