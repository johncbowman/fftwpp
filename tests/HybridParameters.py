def emptyList():
  return []

class ParameterCollection():
# Holds a collection of Params for a given program

  def __init__(self, fillParameters=emptyList, *optArgs):
    # fillParameters: A funtion that returns a list of Parameters.
    #
    # optArgs: Optional arguments for fillParameters. If None, then
    # fillParameters is called without arguments.

    self.optArgs=optArgs
    if optArgs != None:
      vals=fillParameters(*optArgs)
    else:
      vals=fillParameters()

    if type(vals) == list:
      for value in vals:
        if type(value) != Parameters:
          exit("fillOptValues must return a list of OptValues.")
      self.vals=vals
    else:
      exit("fillOptValues must return a list of OptValues.")

  # returns the L value of the Params v
  def byL(self, v):
    return v.L

  # sorts self.val by the value of L
  def sortL(self, reverse=False):
    self.vals.sort(reverse=reverse,key=self.byL)

  # Returns a list of L values (respecting the order of self.val)
  def L(self):
    values=[]
    for v in self.vals:
      values.append(v.L)
    return values

  # Returns a list of M values (respecting the order of self.val)
  def M(self):
    values=[]
    for v in self.vals:
      values.append(v.M)
    return values

  # Returns a list of m values (respecting the order of self.val)
  def m(self):
    values=[]
    for v in self.vals:
      values.append(v.m)
    return values

  # Returns a list of p values (respecting the order of self.val)
  def p(self):
    values=[]
    for v in self.vals:
      values.append(v.p)
    return values

  # Returns a list of q values (respecting the order of self.val)
  def q(self):
    values=[]
    for v in self.vals:
      values.append(v.q)
    return values

  # Returns a list of C values (respecting the order of self.val)
  def C(self):
    values=[]
    for v in self.vals:
      values.append(v.C)
    return values

  # Returns a list of S values (respecting the order of self.val)
  def S(self):
    values=[]
    for v in self.vals:
      values.append(v.S)
    return values

  # Returns a list of D values (respecting the order of self.val)
  def D(self):
    values=[]
    for v in self.vals:
      values.append(v.D)
    return values

  # Returns a list of I values (respecting the order of self.val)
  def I(self):
    values=[]
    for v in self.vals:
      values.append(v.I)
    return values

  # Returns a dictionary of lists of values (respecting the order of self.val)
  def allParms(self):
    params={}
    params["L"]=self.L()
    params["M"]=self.M()
    params["m"]=self.m()
    params["p"]=self.p()
    params["q"]=self.q()
    params["C"]=self.C()
    params["S"]=self.S()
    params["D"]=self.D()
    params["I"]=self.I()
    return params

class Parameters():
# A collectuion of parameters for a hybrid dealiased convolution

  def __init__(self,L,M,m,p,q,C,S,D,I):
    self.L=int(L)
    self.M=int(M)
    self.m=int(m)
    self.p=int(p)
    self.q=int(q)
    self.C=int(C)
    self.S=int(S)
    self.D=int(D)
    self.I=int(I)
