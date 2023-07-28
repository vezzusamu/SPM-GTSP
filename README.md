# Travelling-Salesman-Problem
Genetic algorithm in C++ to solve the TSP problem.

## Abstract
The Traveling Salesman Problem [TSP] is an NP-Hard problem, 
its aim is to find the shortest path to visit a set of cities 
exactly once and return to the starting city. Because of its complexity, 
it cannot be solved in polynomial time. The solution will be approximated, 
in the case of this project a genetic algorithm has been used.
The algorithm starts from a randomly generated population,
where each chromosome represents a possible path, and for each iteration, 
it applies three genetic operators to the population.
The three genetic operators applied are selection,
crossover, and mutation. 
The algorithm has been implemented in a Sequential version, 
a Native threads version and a version based on FastFlow.

