#include <cmath>
#include <iostream>
#include <algorithm>
#include <iterator>
#include <omp.h>
#include <tuple>
#define LEMNISCATE_END 6.2832

std::tuple<double, double> lemniscateDrive(bool switchLane, double t);
double lemniscateUtil(float t);
void trafficLight(double timeTaken, bool* green);
float calculateDistance(std::tuple<double, double> car1, std::tuple<double, double> car2);

int main() {
    struct timespec start{}, end{};
    struct car{
        std::tuple<double, double> pos;
        bool isDriving = true;
        bool lane = false;
        double t = 0;
    };

    bool isGreen = false;
    bool getUpdate = false;
    car cars[6];

    // ascii art for the track
    std::array<std::string, 25> track;
    for (int i = 0; i < 25; i++)
        track[i] = "                                                  ";
    double k = 0;
    while(k < LEMNISCATE_END)
    {
        std::tuple<double, double> pos = lemniscateDrive(false, k);
        int x = (int)std::round(get<0>(pos)) + 10;
        int y = (int)std::round(get<1>(pos)) + 4;
        track[x][y] = '.';
        pos = lemniscateDrive(true, k);
        int i = (int)std::round(get<0>(pos)) + 9;
        int j = (int)std::round(get<1>(pos)) + 5;
        track[i][j] = '.';
        k += 0.1;
    }
    std::array<std::string, 25> trackTemplate = track;

    clock_gettime(CLOCK_MONOTONIC, &start);
    omp_set_num_threads(6);
    #pragma omp parallel default(none) shared(start, end, isGreen, getUpdate, cars, track, std::cout, trackTemplate)
    {
        srand((unsigned) time(nullptr) + omp_get_thread_num()); // seed the random number generator
        int acceleration = rand() % 5 + 1;
        double speed = 0;
        int maxSpeed = rand() % 20 + 10;
        double t = 0; // private variable for each thread to update the lemniscate curve
        bool applyBreak = false;

        bool doOnce = true; // used to print the track only once
        double timeTaken = 0;
        while((int)timeTaken < 300)
        {
            clock_gettime(CLOCK_MONOTONIC, &end);
            double lastTimeTaken = timeTaken;
            timeTaken = (end.tv_sec - start.tv_sec)* 1e9;                   // time in seconds
            timeTaken = (timeTaken + (end.tv_nsec - start.tv_nsec)) * 1e-9; // time in nanoseconds
            double deltaTime = timeTaken - lastTimeTaken;                   // time since the last while loop

            // thread 0 prints the status of all threads every 10 seconds
            if(omp_get_thread_num() == 0)
            {
                if((int)(timeTaken) % 10 == 0)
                    getUpdate = true;
            }
            else
            {
                if(speed < maxSpeed)
                    speed += acceleration * deltaTime * 0.05;

                trafficLight(timeTaken, &isGreen);
                // stop car at the traffic light
                if ((t >= 1.36 && t <= 1.56 && !isGreen) || (t >= 4.5 && t <= 4.7 && isGreen) || applyBreak)
                    cars[omp_get_thread_num()].isDriving = false;
                else
                {
                    t += speed * deltaTime;
                    cars[omp_get_thread_num()].isDriving = true;
                    cars[omp_get_thread_num()].t = t;
                }
                // this loops back the car to the start of the lemniscate
                if(t >= LEMNISCATE_END)
                    t = 0;

                // find the closest car in the current lane
                float closestDistance = 1000;
                double closestT = -1;
                bool isDriving;
                for(auto & car : cars)
                {
                    if (car.pos != cars[omp_get_thread_num()].pos && car.lane == cars[omp_get_thread_num()].lane)
                    {
                        float distance = calculateDistance(cars[omp_get_thread_num()].pos, car.pos);
                        if (distance < closestDistance)
                        {
                            closestDistance = distance;
                            closestT = car.t;
                            isDriving = car.isDriving;
                        }
                    }
                }

                // apply breaks if the car in front is not moving (a car may stop at the traffic light)
                if (closestDistance < 1 && !isDriving && cars[omp_get_thread_num()].t < closestT)
                    applyBreak = true;
                else
                    applyBreak = false;

                // switch lane if car in front and car speed is faster than the car in front
                if (closestDistance < 1 && cars[omp_get_thread_num()].t < closestT)
                    cars[omp_get_thread_num()].lane = !cars[omp_get_thread_num()].lane;

                // update the position of the car using the lemniscate curve
                cars[omp_get_thread_num()].pos = lemniscateDrive(cars[omp_get_thread_num()].lane, t+omp_get_thread_num());

                // print the status of the car
                if (getUpdate && doOnce)
                {
                    int x = (int)std::round(get<0>(cars[omp_get_thread_num()].pos));
                    int y = (int)std::round(get<1>(cars[omp_get_thread_num()].pos));
                    // print the car in the track
                    if(!cars[omp_get_thread_num()].lane)
                        track[x+10][y+4] = '*';
                    else
                        track[x+9][y+5] = '*';

                    // single thread prints the track
                    #pragma omp single nowait
                    {
                        for (int i = 0; i < 25; i++)
                            std::cout<< track[i] << std::endl;
                    };

                    doOnce = false;
                    getUpdate = false;
                }
            }
            if((int)(timeTaken+1) % 12 == 0)
            {
                doOnce = true;
                //reset the track to update in the next round
                track = trackTemplate;
            }
        }
    };

    return 0;
}

std::tuple<double, double> lemniscateDrive(bool switchLane, double t)
{
    double x, y;
    if (!switchLane)
    {
        x = 10*std::cos(t) / (1 + pow(std::sin(t),2));
        y = 10*std::cos(t)*std::sin(t) / (1 + pow(std::sin(t),2));
        return {x, y};
    }
    x = lemniscateUtil(t) * 13.5*std::cos(t) / (1 + pow(std::sin(t),2)) - 2;
    y = lemniscateUtil(t) * 13.5*std::cos(t)*std::sin(t) / (1 + pow(std::sin(t),2));
    return {x, y};
}

double lemniscateUtil(float t)
{
    return 0.5*pow(std::cos(t/2),2) + 0.5;
}

void trafficLight(double timeTaken, bool* green)
{
    if ((int)timeTaken % 2 == 0)
        *green = true;
    if ((int)timeTaken % 4 == 0)
        *green = false;
}

float calculateDistance(std::tuple<double, double> car1, std::tuple<double, double> car2) {
    return sqrt(pow(get<0>(car1) - get<0>(car2), 2) + pow(get<1>(car1) - get<1>(car2), 2));
}
