#include "TaxiTask.h"
#include "common_function.h"
using namespace std;

pthread_mutex_t TaxiTask::mutex = PTHREAD_MUTEX_INITIALIZER;
map<string, Station>* TaxiTask::stationMap = nullptr;
map<string, map<int, set<string>>>* TaxiTask::zoneMap = nullptr;

TaxiTask::TaxiTask(Point p, string c, int i) :
   point(p), card(c), zoneIndex(i) 
{
}
void TaxiTask::run()
{
    auto ite = stationMap->begin();
    while(ite != stationMap->end())
    {
        auto &set = zoneMap->at(ite->first).at(zoneIndex);
        if(set.find(card) == set.end())
        {
            auto distance = point.getDistance(ite->second.position);
            if (distance <= MAX_DISTANCE)
            {
                pthread_mutex_lock(&mutex);
                set.insert(card);
                pthread_mutex_unlock(&mutex);
            }
        }
        ite++;
    }
}
