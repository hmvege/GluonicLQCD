import numpy as np, sys
from operator import itemgetter

class Index: # General method for getting contiguous memory allocation
	def __init__(self,N):
		self.N = N
		self.N_length = len(N)
		
	def __call__(self,indices):
		# if len(indices) != self.N_length:
		# 	raise ValueError("Number of indices %d do not correspond with number of dimensions %d." % (len(indices),self.N_length))
		# return self.N[3]*(self.N[2]*(self.N[1]*indices[0] + indices[1]) + indices[2]) + indices[3]
		# offset = int(0);
		# for i in range(self.N_length):
		# 	prod = 1
		# 	for j in range(i+1,self.N_length):
		# 		prod *= self.N[j]
		# 	offset += indices[i]*prod

		# offset = self.N[3]*(self.N[2]*(self.N[1]*indices[0] + indices[1]) + indices[2]) + indices[3]
		offset = indices[0] + self.N[0]*(indices[1] + self.N[1]*(indices[2] + self.N[2]*indices[3]))
		return offset

class Lattice:
	def __init__(self,N):
		if type(N) != list: raise ValueError("Dimension to be passed as elements in a list.")
		self.N = N

	def printLattice(self,reversed=False,storeInFile=True):
		# Redirecting output
		if storeInFile:
			orig_stdout = sys.stdout
			f = open('lattice.txt', 'w')
			sys.stdout = f

		index = Index(self.N)
		memory_position_string = []
		if not reversed:
			msg = "XYZT order"
		else:
			msg = "TZYX order"
		scr_len = len("[%d,%d,%d,%d = %3d]" % (0,0,0,0,index([0,0,0,0])))*4+4
		str_len = ((len("[%d,%d,%d,%d = %3d]" % (0,0,0,0,index([0,0,0,0])))*4+3) - len(msg))/2
		print "="*str_len + msg + "="*(str_len+1)

		if not reversed:
			for x in xrange(self.N[0]):
				print "X = %d," % x,
				for y in xrange(self.N[1]):
					print "Y = %d" % y
					for z in xrange(self.N[2]):
						for t in xrange(self.N[3]):
							memory_position_string.append([index([x,y,z,t]),(x,y,z,t)])
							print_string = "[%d,%d,%d,%d = %3d]" % (x,y,z,t,memory_position_string[-1][0])
							print print_string,
						if ((x+1)%(self.N[0]) == 0 and (y+1)%(self.N[1]) == 0 and (z+1)%(self.N[2]) == 0 and (t+1)%(self.N[3]) == 0):
							print "\n","="*(scr_len-1)
						print "\n",
					print "\n",
				print "\n",
		else:
			for t in xrange(self.N[3]):
				print "T = %d," % t,
				for z in xrange(self.N[2]):
					print "Z = %d" % z
					for y in xrange(self.N[1]):
						for x in xrange(self.N[0]):
							memory_position_string.append([index([x,y,z,t]),(x,y,z,t)])
							print_string = "[%d,%d,%d,%d = %3d]" % (x,y,z,t,memory_position_string[-1][0])
							print print_string,
						if ((x+1)%(self.N[0]) == 0 and (y+1)%(self.N[1]) == 0 and (z+1)%(self.N[2]) == 0 and (t+1)%(self.N[3]) == 0):
							print "\n","="*(scr_len-1)
						print "\n",
					print "\n",
				print "\n",
		
		# Closing output file
		if storeInFile:
			sys.stdout = orig_stdout
			f.close()

		# Storing memory string
		self.memory_position_string = memory_position_string



class SubLattice:
	def __init__(self,N,numprocs):
		if type(N) != list: raise ValueError("Dimension to be passed as elements in a list.")
		self.N = [n for n in N] # Lattice dimensions
		self.NSub = [n for n in N]

		self.numprocs = numprocs
		restProc = numprocs
		self.indexSub = Index(self.NSub)
		self.indexTot = Index(self.N)

		# Sets up the sub-lattice dimensions
		while restProc >= 2:
			for i in range(0,4):
				self.NSub[i] /= 2;
				restProc /= 2;
				if (restProc < 2): break
		
		for dim in self.NSub: # Error checking
			if dim <= 2:
				print "Error: sublattice dimension less than 2:", self.NSub
				exit(1)

		# Sets up processors per dimension
		self.procsPerDim = []
		for i in range(4):
			self.procsPerDim.append(self.N[i] / self.NSub[i])

		# Sets up the volumes
		self.VSub = [self.NSub[0], self.NSub[1]*self.NSub[0],self.NSub[2]*self.NSub[1]*self.NSub[0],self.NSub[3]*self.NSub[2]*self.NSub[1]*self.NSub[0]]
		self.V = [self.N[0], self.N[0]*self.N[1], self.N[0]*self.N[1]*self.N[2], self.N[0]*self.N[1]*self.N[2]*self.N[3]]
		self.VProc = [	self.procsPerDim[0], 
						self.procsPerDim[0]*self.procsPerDim[1],
						self.procsPerDim[0]*self.procsPerDim[1]*self.procsPerDim[2],
						self.procsPerDim[0]*self.procsPerDim[1]*self.procsPerDim[2]*self.procsPerDim[3]]

		# Sets up processor positions in the lattice
		self.processor_coordinates = []
		for rank in range(numprocs):
			processor_coordinate = [rank % self.procsPerDim[0],
									(rank / self.VProc[0]) % self.procsPerDim[1],
									(rank / self.VProc[1]) % self.procsPerDim[2],
									(rank / self.VProc[2]) % self.procsPerDim[3]]
			self.processor_coordinates.append(processor_coordinate)

		# # Small diagnostics
		# for p in range(numprocs):
		# 	print "===== Processor: %2d =====" % p
		# 	for i in range(4):
		# 		print "Dimension: ", i,
		# 		print "Processor cord: ", self.processor_coordinates[p][i],
		# 		print "Sublattice length: ", self.NSub[i],
		# 		print "Total volume: ", self.V[i]
		# 	print "="*25,"\n"

		# print "t: ", self.NSub[3] / self.NSub[2]
		# print "z: ", self.NSub[2] / self.NSub[1]
		# print "y: ", self.NSub[1] / self.NSub[0]

		print "Lattice dimensions(self.N):                 ", self.N
		print "Volumes of lattice(self.V):                 ", self.V
		print "Sub-lattice dimensions(self.NSub):          ", self.NSub
		print "Volumes of sub-lattice(self.VSub):          ", self.VSub
		print "Number of processsors(self.numprocs):       ", self.numprocs
		print "Processors per dimension(self.procsPerDim): ", self.procsPerDim
		print "Volumes of processsors(self.VProc):         ", self.VProc
		# print "Processor coordinates: \n",self.processor_coordinates

	def printTotalLattice(self,storeInFile=True):
		# Redirecting output
		if storeInFile:
			orig_stdout = sys.stdout
			f = open('all_sub_lattices.txt', 'w')
			sys.stdout = f

		memory_position_string = [] # This can be compared later with the scalar version. If they are equal, the methods are working and the bug is not due to wrong memory placement.
		x_offset, y_offset, z_offset, t_offset = 0,0,0,0

		for processor in range(self.numprocs):
			str_processor =  "Processor: %2d" % processor
			print str_processor
			msg = "TZYX order"
			scr_len = len("[%4d,%4d,%4d,%4d = %5d]" % (0,0,0,0,self.indexSub([1,1,1,1])))*self.NSub[0]+self.NSub[0]
			str_len = ((len("[%4d,%4d,%4d,%4d = %5d]" % (0,0,0,0,self.indexSub([1,1,1,1])))*self.NSub[0]+1) - len(msg))/2
			str_header = "="*str_len + msg + "="*(str_len+1)
			print str_header

			for t in xrange(self.NSub[3]):
				t_offset = (self.processor_coordinates[processor][3] * self.NSub[3] + t)
				# t_offset = (self.processor_coordinates[processor][3] * self.NSub[3] + t) * self.procsPerDim[3]
				# t_offset = self.V[2] * (self.processor_coordinates[processor][3] * self.NSub[3] + t)
				str_t_cord = "T = %d," % t
				print str_t_cord,
				for z in xrange(self.NSub[2]):
					z_offset = self.V[0] * (self.processor_coordinates[processor][2] * self.NSub[2] + z)
					# z_offset = self.V[0] * (self.processor_coordinates[processor][2] * self.NSub[2] + z) * self.procsPerDim[2] + t_offset
					# z_offset = self.V[0] * (self.processor_coordinates[processor][2] * self.NSub[2] + z) * self.NSub[1] / self.NSub[0] + t_offset
					# z_offset = self.V[1] * (self.processor_coordinates[processor][2] * self.NSub[2] + z) + t_offset
					str_z_cord = "Z = %d" % z
					print str_z_cord
					for y in xrange(self.NSub[1]):
						y_offset = self.V[1] * (self.processor_coordinates[processor][1] * self.NSub[1] + y)
						# y_offset = self.V[1] * (self.processor_coordinates[processor][1] * self.NSub[1] + y) * self.procsPerDim[1] + z_offset
						# y_offset = self.V[1] * (self.processor_coordinates[processor][1] * self.NSub[1] + y) * self.NSub[2] / self.NSub[1] * self.NSub[2] / self.NSub[0]+ z_offset
						# y_offset = self.V[0] * (self.processor_coordinates[processor][1] * self.NSub[1] + y) + z_offset
						for x in xrange(self.NSub[0]):
							x_offset = self.V[2] * (self.processor_coordinates[processor][0] * self.NSub[0] + x)
							offset = t_offset + z_offset + y_offset + x_offset
							# x_offset = self.V[2] * (self.processor_coordinates[processor][0] * self.NSub[0] + x) * self.NSub[3] / self.NSub[2] * self.NSub[2] / self.NSub[1] * self.NSub[1] / self.NSub[0] + y_offset
							# x_offset = (self.processor_coordinates[processor][0] * self.NSub[0] + x) + y_offset
							memory_position_string.append([self.indexTot([x+self.NSub[0]*self.processor_coordinates[processor][0],y+self.NSub[1]*self.processor_coordinates[processor][1],z+self.NSub[2]*self.processor_coordinates[processor][2],t+self.NSub[3]*self.processor_coordinates[processor][3]]),(x+self.NSub[0]*self.processor_coordinates[processor][0],y+self.NSub[1]*self.processor_coordinates[processor][1],z+self.NSub[2]*self.processor_coordinates[processor][2],t+self.NSub[3]*self.processor_coordinates[processor][3]),processor])
							# memory_position_string.append([self.indexTot([x_offset,y_offset,z_offset,t_offset]),(x+self.NSub[0]*self.processor_coordinates[processor][0],y+self.NSub[1]*self.processor_coordinates[processor][1],z+self.NSub[2]*self.processor_coordinates[processor][2],t+self.NSub[3]*self.processor_coordinates[processor][3]),processor])
							print_string = "[%d,%d,%d,%d = %3d]" % (x,y,z,t,memory_position_string[-1][0]) # Memory positions
							print print_string,
						if ((x+1)%(self.NSub[0]) == 0 and (y+1)%(self.NSub[1]) == 0 and (z+1)%(self.NSub[2]) == 0 and (t+1)%(self.NSub[3]) == 0):
							print "\n","="*(scr_len-1)
						print "\n",
					print "\n",
				print "\n",

		if storeInFile:
			sys.stdout = orig_stdout
			f.close()

		self.memory_position_string = memory_position_string


	def printSubLattice(self, reversed = False, storeInFile = True):
		# Redirecting output
		if storeInFile:
			orig_stdout = sys.stdout
			f = open('sublattice.txt', 'w')
			sys.stdout = f

		for processor in range(self.numprocs):
			print "Processor: ", processor
			if not reversed:
				msg = "XYZT order"
			else:
				msg = "TZYX order"
			scr_len = len("[%d,%d,%d,%d = %3d]" % (0,0,0,0,self.indexSub([0,0,0,0])))*self.NSub[0]+self.NSub[0]
			str_len = ((len("[%d,%d,%d,%d = %3d]" % (0,0,0,0,self.indexSub([0,0,0,0])))*self.NSub[0]+1) - len(msg))/2
			print "="*str_len + msg + "="*(str_len+1)

			if not reversed:
				for x in xrange(self.NSub[0]):
					print "X = %d," % x,
					for y in xrange(self.NSub[1]):
						print "Y = %d" % y
						for z in xrange(self.NSub[2]):
							for t in xrange(self.NSub[3]):
								print_string = "[%d,%d,%d,%d = %3d]" % (x,y,z,t,self.indexSub([x,y,z,t]))
								print print_string,
							if ((x+1)%(self.NSub[0]) == 0 and (y+1)%(self.NSub[1]) == 0 and (z+1)%(self.NSub[2]) == 0 and (t+1)%(self.NSub[3]) == 0):
								print "\n","="*(scr_len-1)
							print "\n",
						print "\n",
					print "\n",
			else:
				for t in xrange(self.NSub[3]):
					print "T = %d," % t,
					for z in xrange(self.NSub[2]):
						print "Z = %d" % z
						for y in xrange(self.NSub[1]):
							for x in xrange(self.NSub[0]):
								print_string = "[%d,%d,%d,%d = %3d]" % (x,y,z,t,self.indexSub([x,y,z,t]))
								print print_string,
							if ((x+1)%(self.NSub[0]) == 0 and (y+1)%(self.NSub[1]) == 0 and (z+1)%(self.NSub[2]) == 0 and (t+1)%(self.NSub[3]) == 0):
								print "\n","="*(scr_len-1)
							print "\n",
						print "\n",
					print "\n",

		if storeInFile:
			sys.stdout = orig_stdout
			f.close()

def testIfEqualMemoryPositions(arr1,arr2):
	if sum([True if i==j else False for i,j in zip(arr1, arr2)]) == len(arr2):
		print "Success: Equal memory positions."
	else:
		print "Failure: Not equal memory positions."

def main():
	# CONSTANTS
	dimensions = [4,4,4,8]
	dimensions = map(lambda x: 12*x, dimensions)
	numprocs = 512
	storeOutput = True
	max_index = np.prod(dimensions)
	index_ceiling = max_index/8

	lattice = Lattice(dimensions)
	lattice.printLattice(True, storeInFile = storeOutput)
	sublattice = SubLattice(dimensions,numprocs)
	# sublattice.printSubLattice(storeInFile = storeOutput)
	sublattice.printTotalLattice(storeInFile = storeOutput)

	# parallel32core = np.loadtxt("parallel_index_file_32cores.txt",dtype=int)
	# parallel32core = np.asarray(sorted(parallel32core,key=itemgetter(3,2,1,0)))

	# parallel16core = np.loadtxt("parallel_index_file_16cores.txt",dtype=int)
	# parallel16core = np.asarray(sorted(parallel16core,key=itemgetter(3,2,1,0)))
	# para_sum = sum(parallel32core[:,-1] == parallel16core[:,-1])
	# print para_sum
	# print para_sum == (len(parallel32core) and len(parallel16core))


	# parallel32core = np.loadtxt("para32ranks.txt",dtype=int)
	# parallel32core = np.asarray(sorted(parallel32core,key=itemgetter(3,2,1,0)))

	# parallel16core = np.loadtxt("para16ranks.txt",dtype=int)
	# parallel16core = np.asarray(sorted(parallel16core,key=itemgetter(3,2,1,0)))
	# para_sum = sum(parallel32core[:,-1] == parallel16core[:,-1])
	# print para_sum
	# print para_sum == (len(parallel32core) and len(parallel16core))


	parallelIndexes = [i[1] for i in sublattice.memory_position_string]
	parallelLocations = np.asarray([i[0] for i in sublattice.memory_position_string])
	scalarIndexes = np.asarray([i[1] for i in lattice.memory_position_string])
	scalarLocations = np.asarray([i[0] for i in lattice.memory_position_string])

	# Sorting positions
	mega_list = zip(parallelIndexes,parallelLocations,[i[-1] for i in sublattice.memory_position_string])
	mega_array = np.asarray(sorted(mega_list,key=lambda i: itemgetter(0)(i)[::-1])) # Sorting the index ordering
	memoryDifferences = mega_array[:,1] - scalarLocations[:]

	# # Comparing para32 proc positions with what we should have
	# para32WithProcNames = np.loadtxt("para32withProcNames.txt",dtype=int)
	# para32WithProcNames = np.asarray(sorted(para32WithProcNames,key=itemgetter(3,2,1,0)))

	# counter = 0
	# for elem1, elem2 in zip(para32WithProcNames,mega_array):
	# 	for index1,index2 in zip(elem1[0:4],elem2[0]):
	# 		if index1 != index2:
	# 			print "Error in index position!"
	# 			counter += 1
	# 	if elem1[-1] != elem2[-1]:
	# 		print "Error in processor position!"
	# if counter != 0:
	# 	print counter


	# print "Printing the parallel and scalar memory positions: "
	# for i,P,parallelIndex,x,scalarIndex,y,memDiff in zip(	range(index_ceiling),
	# 											mega_array[:,-1], # Processor who performed calculation
	# 											# mega_array[:,0], # Parallel index
	# 											# mega_array[:,1], # Parallel mem loc
	# 											parallel32core[:index_ceiling,0:4], # Parallel mem loc
	# 											parallel32core[:index_ceiling,-1], # Parallel index
	# 											scalarIndexes[:index_ceiling], # Scalar index
	# 											scalarLocations[:index_ceiling],
	# 											memoryDifferences): # Scalar memloc
	# 	print "%4d: Parallel(P: %2d xyzt index: %16s): %6d || Scalar(xyzt index: %16s: %6d || Mem.diff: %4d" % (i,P,parallelIndex,x,scalarIndex,y,memDiff)

	bool_sums_equal = sum(parallelLocations) == sum(scalarLocations)
	bool_elements_equal = (sum([True if i in scalarLocations else False for i in parallelLocations]) == len(sublattice.memory_position_string))
	if bool_sums_equal and bool_sums_equal:
		print "Success: Equal number of elements accessed."
	else:
		print "Failure: Unequal number of elements accessed"

	# Testing if we are mapping parallel processors to scalar memory allocation
	# testIfEqualMemoryPositions(parallel32core[:,-1],mega_array[:,1])
	# testIfEqualMemoryPositions(scalarLocations,mega_array[:,1])

	# Comparing memory locations to the parallel and scalar branch of program run on a 8**4 lattice
	# scalarVersionIndices = np.loadtxt("scalar16cubed16_indexes.txt",usecols=(1,2,3,4,6),dtype=int) # Only for 8**4 lattice
	# if sum([True if i==j else False for i,j in zip(scalarVersionIndices[:,-1], mega_array[:,1])]) == len(mega_array[:,1]):
	# 	print "Success: Equal memory positions."
	# else:
	# 	print "Failure: Not equal memory positions."

if __name__ == '__main__':
	main()