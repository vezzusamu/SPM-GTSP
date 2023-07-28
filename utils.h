//
// Created by s.vezzuto on 6/17/23.
//



#ifndef TRAVELLING_SALESMAN_PROBLEM_MASTER_UTILS_H
#define TRAVELLING_SALESMAN_PROBLEM_MASTER_UTILS_H
#include <iostream>
#include <algorithm>
#include <random>
class Utils {
    private:
        int** citiesDistances;
        int numberOfCities;
        int seed;
        mt19937 generator;


        void initRandomGenerator() {
            random_device rd;
            if (seed == 0) {
                seed = rd();
            }
            generator = mt19937(seed);
        }


        void initCitiesDistances()
        {
            citiesDistances = new int*[numberOfCities];
            for(int i=0; i < numberOfCities; ++i)
            {
                citiesDistances[i] = new int[numberOfCities];
            }
        }

        //We connect two cities by adding a bidirectional edge between them
        void connectTwoCities(int u, int v, int c)
        {
            citiesDistances[u][v] = c;
            citiesDistances[v][u] = c;
        }




    public:
        Utils(int seed = 0){
            this->seed = seed;
            initRandomGenerator();
        }


        void createCitiesGraphWithRandomDistances(int numberOfCities){
            this->numberOfCities = numberOfCities;
            initCitiesDistances();
            std::uniform_int_distribution<int> unif(1, 100);
            for(int i=0; i < numberOfCities; ++i)
            {
                for(int j=i+1; j < numberOfCities; ++j)
                {
                    connectTwoCities(i, j, unif(generator));
                }
            }
        }

        void printCities(){
            for (int i = 0; i < numberOfCities; ++i) {
                for (int j = 0; j < numberOfCities; ++j) {
                    cout << citiesDistances[i][j] << " ";
                }
                cout << endl;
            }
        }

        int** getCitiesDistances(){
            return citiesDistances;
        }

};


#endif //TRAVELLING_SALESMAN_PROBLEM_MASTER_UTILS_H
