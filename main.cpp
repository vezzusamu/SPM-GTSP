//
// Created by Samuele Vezzuto on 02/06/23.
//

#include <iostream>
#include <algorithm>
#include <random>
#include "calculator.h"
#include "parallel_calculator.h"
#include "fastflow_calculator.h"
#include "sequential_calculator.h"
#include "utils.h"


using namespace std;

int maxIterations = 0;
int nextPercentageToPrint = 0;

void printCompletionPercentage(int timeElapsed, int iterationNumber) {
    int actualPercentage = (iterationNumber * 100) / maxIterations;
    if (actualPercentage > nextPercentageToPrint) {
        cout << "Percentage of completion: " << actualPercentage << "%. \nPerformed in: "<< timeElapsed << " ms." << endl;
        nextPercentageToPrint += 10;
    }
}


int main(int argc, char *argv[]) {
    Utils utils = Utils();

    //Default values
    int chromosomeDimension = 2000;
    int calculatorType = 1;
    int numberOfWorkers = 10;
    int numberOfCities = 1000;
    int mutationPercentage = 30;
    maxIterations = 10;

    if (argc > 6) {
        calculatorType = atoi(argv[1]);
        numberOfWorkers = atoi(argv[2]);
        chromosomeDimension = atoi(argv[3]);
        numberOfCities = atoi(argv[4]);
        maxIterations = atoi(argv[5]);
        mutationPercentage = atoi(argv[6]);
    }

    mutationPercentage = (mutationPercentage < 100 && mutationPercentage >= 0) ? mutationPercentage : 10;
    cout << "Calculate the minimal path length by using genetic algorithm." << endl;
    cout << "Number of cities: " << numberOfCities << endl;
    cout << "Number of chromosome: " << chromosomeDimension << endl;
    cout << "Number of iterations: " << maxIterations << endl;
    cout << "Mutation percentage: " << mutationPercentage << " %" << endl;
    utils.createCitiesGraphWithRandomDistances(numberOfCities);
    auto start = std::chrono::system_clock::now();

    Calculator* calculator;

    switch (calculatorType) {
        case 0:
            calculator = new SequentialCalculator(chromosomeDimension);
            numberOfWorkers = 1;
            break;
        case 1:
            calculator = new ParallelCalculator(numberOfWorkers, chromosomeDimension);
            break;
        case 2:
            calculator = new FastFlowCalculator(numberOfWorkers, chromosomeDimension);
            break;
        default:
            calculator = new SequentialCalculator(chromosomeDimension);
            numberOfWorkers = 1;
    }


    calculator->setStartingTime(start);
    calculator->setNumberOfCities(numberOfCities);
    calculator->setCitiesDistances(utils.getCitiesDistances());


    calculator->generateFirstChromosomes();
    //Elitism: we keep the best path of the previous generation
    calculator->evaluateAndSortChromosomes();

    int iterationNumber = 1;
    while((iterationNumber < maxIterations))
    {
        calculator->calculateChromosomesCrossover();
        calculator->calculateChromosomesMutations(mutationPercentage);
        calculator->evaluateAndSortChromosomes();
        printCompletionPercentage(calculator->getTimeElapsed(), iterationNumber);
        iterationNumber++;
    }
    auto end = std::chrono::system_clock::now();
    cout << "Calculations completed." << endl;
    calculator->printTotalTimeSpentForCalculations();
    cout << "Minimal path length calculated: " << calculator->getBestPathLength() << endl;
    std::cout << "Total time spent using "<< calculator->getCalculatorType() << calculator->getNumberOfThreads() << ": " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms" << std::endl;
    std::ofstream outputFile("output.txt", std::ios::app);
    if (outputFile.is_open()) {
        // Write data to the file
        outputFile << calculator->getCalculatorType() << "," << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "," << calculator->getElapsedTime() << "," << numberOfCities << "," << chromosomeDimension << "," << maxIterations <<"," << numberOfWorkers << std::endl;
        outputFile.close();
        std::cout << "Data written to the file successfully." << std::endl;
    }
    else {
        std::cout << "Failed to open the file." << std::endl;
    }
    return 0;
}


