import sys

if len(sys.argv) < 1:
	print 'please give file name' 
else:
	task1max = 0
	task2max = 0
	with open(sys.argv[1],'r') as f:
		for line in f:
			if 'Task1' in line: 
				numbers = line.split(' ')
				if len(numbers) < 3:
					print 'invalid line'
				else:
					if task1max < int(numbers[2]):
						task1max = int(numbers[2])
			if 'Task2' in line: 
				numbers = line.split(' ')
				if len(numbers) < 3:
					print 'invalid line'
				else:
					if task2max < int(numbers[2]):
						task2max = int(numbers[2])
	print 'Max for task1:' + str(task1max)
	print 'Max for task2:' + str(task2max)

				
 
