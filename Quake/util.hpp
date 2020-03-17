#pragma once

#include "debugapi.h"
#include "q_stdinc.hpp"
#include "cvar.hpp"

#define GLM_FORCE_INLINE
#include <glm.hpp>

#include <array>
#include <string>
#include <cassert>
#include <cmath>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <sstream>

#if !defined(QUAKE_FORCEINLINE)
#if defined(_MSC_VER)
#define QUAKE_FORCEINLINE __forceinline
#elif defined(__GNUC__) && __GNUC__ > 3
// Clang also defines __GNUC__ (as 4)
#define QUAKE_FORCEINLINE inline __attribute__((__always_inline__))
#else
#define QUAKE_FORCEINLINE inline
#endif
#endif

// TODO VR:
template <typename T>
void AngleVectors(const T& angles, T& forward, T& right, T& up);

namespace quake::util
{
    template <typename... Ts>
    [[nodiscard]] std::string stringCat(const Ts&... xs)
    {
        std::ostringstream oss;
        (oss << ... << xs);
        return oss.str();
    }

    template <typename T, typename... Ts>
    [[nodiscard]] std::string stringCatSeparated(
        const std::string_view separator, const T& x, const Ts&... xs)
    {
        std::ostringstream oss;
        oss << x;
        ((oss << separator << xs), ...);
        return oss.str();
    }

    template <typename... Ts>
    void debugPrint(const Ts&... xs)
    {
        OutputDebugStringA(stringCat(xs...).data());
    }

    template <typename... Ts>
    void debugPrintSeparated(const std::string_view separator, const Ts&... xs)
    {
        OutputDebugStringA(stringCatSeparated(separator, xs...).data());
    }

    template <typename T>
    [[nodiscard]] QUAKE_FORCEINLINE constexpr glm::vec3 toVec3(
        const T& v) noexcept
    {
        return {v[0], v[1], v[2]};
    }

    [[nodiscard]] QUAKE_FORCEINLINE constexpr const glm::vec3& toVec3(
        const glm::vec3& v) noexcept
    {
        return v;
    }

    [[nodiscard]] QUAKE_FORCEINLINE constexpr glm::vec3& toVec3(
        glm::vec3& v) noexcept
    {
        return v;
    }

    [[nodiscard]] QUAKE_FORCEINLINE constexpr glm::vec3&& toVec3(
        glm::vec3&& v) noexcept
    {
        return std::move(v);
    }

    QUAKE_FORCEINLINE constexpr void toQuakeVec3(
        vec3_t out, const glm::vec3& v) noexcept
    {
        out[0] = v[0];
        out[1] = v[1];
        out[2] = v[2];
    }

    template <typename... Ts>
    [[nodiscard]] constexpr auto makeAdjustedMenuLabels(const Ts&... labels)
    {
        constexpr auto maxLen = 25;

        assert(((strlen(labels) <= maxLen) && ...));
        return std::array{
            (std::string(maxLen - strlen(labels), ' ') + labels)...};
    }

    template <typename TVec3AMin, typename TVec3AMax, typename TVec3BMin,
        typename TVec3BMax>
    [[nodiscard]] constexpr bool boxIntersection(const TVec3AMin& aMin,
        const TVec3AMax& aMax, const TVec3BMin& bMin, const TVec3BMax& bMax)
    {
        return aMin[0] <= bMax[0] && //
               aMin[1] <= bMax[1] && //
               aMin[2] <= bMax[2] && //
               aMax[0] >= bMin[0] && //
               aMax[1] >= bMin[1] && //
               aMax[2] >= bMin[2];
    }

    [[nodiscard]] inline double lerp(double a, double b, double f) noexcept
    {
        return (a * (1.0 - f)) + (b * f);
    }

    template <typename T>
    [[nodiscard]] constexpr inline auto safeAsin(const T x) noexcept
    {
        assert(!std::isnan(x));
        assert(x >= T(-1));
        assert(x <= T(1));

        return std::asin(x);
    }

    template <typename T>
    [[nodiscard]] constexpr inline auto safeAtan2(const T y, const T x) noexcept
    {
        assert(!std::isnan(y));
        assert(!std::isnan(x));
        assert(y != T(0) || x != T(0));

        return std::atan2(y, x);
    }

    [[nodiscard]] inline glm::vec3 pitchYawRollFromDirectionVector(
        const glm::vec3& xup, const glm::vec3& dir)
    {
        // From: https://stackoverflow.com/a/21627251/598696

        const auto pitch = safeAsin(dir[2]);
        const auto yaw = safeAtan2(dir[1], dir[0]);

        const auto up = toVec3(xup);
        const auto w0 = glm::vec3{-dir[1], dir[0], 0};
        const auto u0 = glm::cross(w0, dir);

        const auto w0len = glm::length(w0);
        assert(w0len != 0);

        const auto u0len = glm::length(u0);
        assert(u0len != 0);

        const auto roll =
            safeAtan2(glm::dot(w0, up) / w0len, glm::dot(u0, up) / u0len);

        auto res = glm::degrees(glm::vec3{pitch, yaw, roll});
        res[0 /* PITCH */] *= -1.f;
        res[2 /* ROLL */] -= 180.f;
        return res;
    }

    struct GlmAngledVectors
    {
        glm::vec3 _forward;
        glm::vec3 _right;
        glm::vec3 _up;
    };

    [[nodiscard]] inline GlmAngledVectors getAngledVectors(
        const glm::vec3& v)
    {
        glm::vec3 forward, right, up;
        AngleVectors(v, forward, right, up);

        return {forward, right, up};
    }

    [[nodiscard]] inline glm::vec3 getDirectionVectorFromPitchYawRoll(
        const glm::vec3& v)
    {
        return getAngledVectors(v)._forward;
    }

    template <typename T>
    [[nodiscard]] auto makeMenuAdjuster(const bool isLeft)
    {
        return [isLeft](
                   const cvar_t& cvar, const T incr, const T min, const T max) {
            const auto newVal =
                static_cast<T>(isLeft ? cvar.value - incr : cvar.value + incr);
            const auto res = static_cast<T>(std::clamp(newVal, min, max));

            Cvar_SetValue(cvar.name, res);
        };
    }

    template <typename T>
    [[nodiscard]] bool hitSomething(const T& trace) noexcept
    {
        return trace.fraction < 1.0f;
    }
} // namespace quake::util

namespace std
{
    template <int D, typename T, glm::qualifier P>
    struct tuple_size<glm::vec<D, T, P>>
        : std::integral_constant<std::size_t, D>
    {
    };

    template <std::size_t I, int D, typename T, glm::qualifier P>
    struct tuple_element<I, glm::vec<D, T, P>>
    {
        using type = T;
    };
} // namespace std

namespace glm
{
    template <std::size_t I, int D, typename T, glm::qualifier P>
    [[nodiscard]] QUAKE_FORCEINLINE T get(const glm::vec<D, T, P>& v) noexcept
    {
        return v[I];
    }
} // namespace glm
