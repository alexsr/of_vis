//
// Alexander Scheid-Rehder
// alexanderb@scheid-rehder.de
// https://www.alexsr.de
// https://github.com/alexsr
//

#pragma once
#include "time.hpp"
#include "logging.hpp"
#include <functional>

namespace tostf
{
    template <typename T, typename = std::enable_if_t<is_duration_v<T>>>
    T profile_func(const std::function<void()>& f) {
        const auto start_time = high_res_clock::now();
        f();
        const auto elapsed_time = high_res_clock::now() - start_time;
        return std::chrono::duration_cast<T>(elapsed_time);
    }

    template <typename T, typename = std::enable_if_t<is_duration_v<T>>>
    typename T::rep profile_func_t(const std::function<void()>& f) {
        return profile_func<T>(std::forward<decltype(f)>(f)).count();
    }

    template <typename T>
    struct Lap {
        T duration;
        T split;
    };

    template <typename T>
    struct Event_profiler {
        Event_profiler();
        void start();
        Lap<T> lap();
        void resume();
        Lap<T> pause();
        T elapsed();
    private:
        void reset();
        steady_time_point _start_time;
        steady_time_point _last_lap;
        steady_time_point _pause_time;
        T _elapsed;
        bool _paused;
    };

    template <typename T>
    std::enable_if_t<is_duration_v<T>> log_event_timing(const std::string& name, const Lap<T>& lap, typename T::rep max = 0) {
        auto lap_bg = cmd::bg_color::h_green;
        if (max <= 0) {
            lap_bg = cmd::bg_color::h_blue;
        }
        else if (lap.duration.count() > max) {
            lap_bg = cmd::bg_color::h_red;
        }
        log_section("Duration of " + name, lap_bg);
        log_tag("LAP  ", lap_bg) << lap.duration.count() << get_duration_string<T>();
        log_tag("SPLIT", lap_bg) << lap.split.count() << get_duration_string<T>();
    }

    template <typename T>
    std::enable_if_t<is_duration_v<T>> log_event_timing(const std::string& name, const T& time, typename T::rep max = 0) {
        auto lap_bg = cmd::bg_color::h_green;
        if (max <= 0) {
            lap_bg = cmd::bg_color::h_blue;
        }
        else if (time.count() > max) {
            lap_bg = cmd::bg_color::h_red;
        }
        log_section("Duration of " + name, lap_bg);
        log_tag("Time  ", lap_bg) << time.count() << get_duration_string<T>();
    }

    template <typename T>
    Event_profiler<T>::Event_profiler() : _paused(true) {
        reset();
    }

    template <typename T>
    void Event_profiler<T>::start() {
        reset();
        resume();
    }

    template <typename T>
    void Event_profiler<T>::reset() {
        _start_time = high_res_clock::now();
        _pause_time = _start_time;
        _last_lap = _start_time;
        _elapsed = T(0);
    }

    template <typename T>
    Lap<T> Event_profiler<T>::lap() {
        if (_paused) {
            return {T(0), std::chrono::duration_cast<T>(_pause_time - _start_time)};
        }
        const auto now = high_res_clock::now();
        Lap<T> lap{std::chrono::duration_cast<T>(now - _last_lap), std::chrono::duration_cast<T>(now - _start_time)};
        _last_lap = now;
        return lap;
    }

    template <typename T>
    void Event_profiler<T>::resume() {
        _paused = false;
        _start_time = high_res_clock::now() - _elapsed;
    }

    template <typename T>
    T Event_profiler<T>::elapsed() {
        auto now = high_res_clock::now();
        if (_paused) {
            now = _pause_time;
        }
        return std::chrono::duration_cast<T>(now - _start_time);
    }

    template <typename T>
    Lap<T> Event_profiler<T>::pause() {
        if (!_paused) {
            _pause_time = high_res_clock::now();
        }
        Lap<T> lap{std::chrono::duration_cast<T>(_pause_time - _last_lap), std::chrono::duration_cast<T>(_pause_time - _start_time)};
        _paused = true;
        return lap;
    }
}
