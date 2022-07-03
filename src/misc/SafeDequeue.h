/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2020  chiheb <email>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SAFEDEQUEUE_H
#define SAFEDEQUEUE_H
#include <memory>
#include "LDDState.h"
#include <queue>
#include <mutex>
#include <functional>
#include <condition_variable>
#include <spot/twa/twagraph.hh>

//class CNDFS;
typedef pair<LDDState*, MDD> couple;
typedef pair<couple, Set> Pair;
typedef pair<LDDState*, int> couple_th;

typedef pair<struct myState_t*, int> coupleSuccessor;

struct empty_queue: std::exception {
    ~empty_queue() {};
    const char* what() const noexcept {return "";}
    
    };
/**
 * @todo write docs
 */
template<typename T>
class SafeDequeue {
    private:
        mutable std::mutex mut;
        std::queue<T> data_queue;
        std::condition_variable data_cond;

    public:
        /**
         * Default constructor
         */
        SafeDequeue();

        /**
         * Copy constructor
         *
         * @param other TODO
         */
        SafeDequeue ( const SafeDequeue& other );

        /**
         * Destructor
         */
        ~SafeDequeue();
        SafeDequeue& operator= ( const SafeDequeue& ) = delete;
        void push ( T new_value );
        bool try_pop ( T& value );
        bool empty() const;
        T& front();
        T& back();
        int size();
    };

#endif // SAFEDEQUEUE_H
