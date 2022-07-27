#ifndef OBSERVER_HPP
#define OBSERVER_HPP

#include <list>

class Listener{
 public:
    virtual void onNotification() = 0;

};


class ChangeNotifier{
 public:

    ChangeNotifier() = default;
    ChangeNotifier(const ChangeNotifier&) = delete;

    inline void notifyListeners(){
        for(auto* listener : listeners_){
            listener->onNotification();
        }
    }

    inline void addListener(Listener* listener){
        listeners_.push_back(listener);
    }

    inline void removeListener(Listener* listener){
        listeners_.remove(listener);
    }
 private:
    std::list<Listener*> listeners_ {};
};

#endif