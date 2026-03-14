#ifndef OBSERVER_H
#define OBSERVER_H

#include <algorithm>
#include <vector>

class ec_observer {
public:
    virtual ~ec_observer() {
    }
    virtual void update() = 0;
};

class ec_observer_subject {
public:
    ec_observer_subject() {
    }
    virtual ~ec_observer_subject() {
    }

    void attach(ec_observer* obs) {
        observers.push_back(obs);
    }
    void detach(ec_observer* obs) {
        observers.erase(std::remove(observers.begin(), observers.end(), obs), observers.end());
    }
    void notify() {
        for (unsigned int i = 0; i < observers.size(); ++i) {
            observers[i]->update();
        }
    }

private:
    std::vector<ec_observer*> observers;
};

#endif
