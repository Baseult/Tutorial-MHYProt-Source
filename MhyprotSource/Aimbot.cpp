#include "Aimbot.h"
#include "Inputs.h"
#include "Offsets.h"
#include "settings.h"
#include "CGraphics.h"
#include "../MhyprotSource/Mhyprot/baseadress.h"
#include <D3D11.h>
#include <d3dx9math.h>
#include <chrono>
#include <complex>
#include <corecrt_math_defines.h>
#include <iostream>

#include "quartic.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "D3dx9.lib")
#pragma comment(lib, "D3DCompiler.lib")


TargetInfo Aimbot::current_target;
bool Aimbot::is_lock = false;

using namespace std;
using namespace std::chrono;

#define MATH_PI         (4.f)

//------------------------------------------------------------------------------------------------------------------------



std::vector<uint64_t> get_entities() //Loop all entities 
{
    std::vector<uint64_t> all_entities;

    const int entitycount = read<int>(world + m_entitylistcount); //Get the number of how many Entities are nearby

    for (uintptr_t i = 0; i < entitycount; i++) { //Loop through each entity

        if (const auto table_size = read<uintptr_t>(entitylist + 0x8 * i); table_size != NULL) {
            all_entities.push_back(table_size); //Add entity to list
        }
    }

    return all_entities;
}

TargetInfo Aimbot::get_best_target() //Get best entity (closest to your crosshair)
{ 
    auto best_target = TargetInfo();

    float best_angle_lenght = FLT_MAX; //get best angle
    float saveddist = 90000000; //get best distance

    for (const auto entity: get_entities()) 
    {
        constexpr float width = 2560; //Enter your screen width
        constexpr float height = 1440; //Enter your screen height

        TargetInfo aimbot_target = get_target_info(entity);

        if (!aimbot_target.isInScreen || aimbot_target.entity == 0) //Check if Target is in Screen and if Target is an entity
            continue;

        const float dist = aimbot_target.screenPositionNoPredict.DistTo(Vector3(width / 2, height / 2, 0)); //Get Distance from the Target to Crosshair

        const float dist2 = Vector3(aimbot_target.screenPosition.x, aimbot_target.screenPosition.y, aimbot_target.screenPosition.z).DistTo(Vector3(width / 2, height / 2, 0));

        if (saveddist > dist && (dist < aimbot_fov || dist2 < aimbot_fov)) {
            saveddist = dist;
            best_target = aimbot_target;
        }

        //if (const float dist_temp = aimbot_target.screenPositionNoPredict.DistTo(Vector3(width / 2, height / 2, 0)); dist_temp < aimbot_fov /*&& dist_temp < best_angle_lenght*/)
        //{
        //	best_angle_lenght = dist_temp;
        //	best_target = aimbot_target;
        //}
    }

    return best_target;
}

//------------------------------------------------------------------------------------------------------------------------

Vector3 calculate_impact_position(const double time, const Vector3 start_position, const Vector3 velocity, const Vector3 accel) { //Calculates the position of the target in x seconds using Velocity and Acceleration.
    return (start_position + (velocity * time) + (0.5 * accel * pow(time, 2)));
}


DComplex *solvequadric(const double a, const double b, const double c) //quadratic equation calculates how much time the rocket needs to reach the target, based on Target Velocity (Without Acceleration)
{
    double x1;

    if (const double discriminant = b * b - 4 * a * c; discriminant > 0) {
        x1 = (-b + sqrt(discriminant)) / (2 * a);
        const double x2 = (-b - sqrt(discriminant)) / (2 * a);
        auto *retval = new DComplex[2];
        retval[0].real(x1);
        retval[1].real(x2);
        return retval;
    } else {
        if (discriminant == 0) {
            x1 = -b / (2 * a);
            const auto retval = new DComplex[1];
            retval[0].real(x1);
            return retval;
        }

        const double real_part = -b / (2 * a);
        const double imaginary_part = sqrt(-discriminant) / (2 * a);
        const auto retval = new DComplex[2];
        retval[0].real(real_part);
        retval[0].imag(imaginary_part);
        retval[1].real(real_part);
        retval[1].imag(imaginary_part);
        return retval;
    }
};

//------------------------------------------------------------------------------------------------------------------------

Vector3 shortacceleration = {0, 0, 0};
int acccount = 0;
float notimeacceleration;

high_resolution_clock::time_point accclock1;
high_resolution_clock::time_point accclock2;
high_resolution_clock::time_point accclock3;
high_resolution_clock::time_point accclock4;

Vector3 accc1 = {0, 0, 0};
Vector3 accc2 = {0, 0, 0};
Vector3 accc3 = {0, 0, 0};
Vector3 accc4 = {0, 0, 0};

bool locked = false;

//---------------------------------------------Acceleration Calculation---------------------------------------------------------------------------


Vector3 acceleration = {0, 0, 0};
float pretime = 0;

Vector3 getacceleration(const Vector3 target_velocity) //Here 2 velocity vectors are stored and compared to find out the acceleration of the target.
{
    
    acccount++;
    if (acccount == 1) {
        accclock1 = high_resolution_clock::now(); //yup i use c++ clocks instead of ingame timer here, easier to handle for multiple games
        accc1 = target_velocity;
    } 
    else if (acccount == 25) //After 25 loops the acceleration is measured. Higher value = more loops and more precise but slower calculation, leaves this between 10 - 100
    {
        const high_resolution_clock::time_point accclock2 = high_resolution_clock::now();
        const Vector3 accc2 = target_velocity;

        const auto time = duration_cast<duration<double>>(accclock2 - accclock1);
        const double timex = time.count();

        if (const double testacceleration = ((accc2 - accc1) / timex).DistTo({0, 0, 0}); testacceleration >= 0.25 || testacceleration <= -0.25) {
            acceleration = (accc2 - accc1) / timex; //Acceleration M/s
        } else {
            acceleration = {0, 0, 0};
        }

        acccount = 0; //Loop reset
    }

    return acceleration; //return acceleration of the target
}



double quadratic_get_time_to_target(const Vector3 destination, const Vector3 source, const double bulletspeed, const Vector3 velocity, double compare) {

    const Vector3 deltapos = destination - source;
    const double t3_calc = velocity.Dot(velocity) - bulletspeed * bulletspeed;
    const double t2_calc = 2.0 * deltapos.Dot(velocity);
    const double tcalc = deltapos.Dot(deltapos);

    const std::complex<double> *solutions = solvequadric(t3_calc, t2_calc, tcalc); //Quadratic equation to calculate the time of the bullet to the target

    double smallest = 10000;
    for (int i = 0; i <= 4; i++) {
        if (const double deltasolution = solutions[i].real(); deltasolution > 0) {
            if (const double test = std::abs(deltasolution - pretime); test < smallest) {
                smallest = test;
            }
        }
    }

    return smallest;
}

double quatric_get_time_to_target(Vector3 destination, Vector3 source, double bulletspeed, Vector3 velocity,
                                  Vector3 targetacceleration) {
    const Vector3 deltapos = destination - source;
    const double t4_calc = (targetacceleration.Dot(targetacceleration)) / 4;
    const double t3_calc = (targetacceleration.Dot(velocity));
    const double t2_calc = targetacceleration.Dot(deltapos) + velocity.Dot(velocity) - bulletspeed * bulletspeed;
    const double t1_calc = 2.0 * deltapos.Dot(velocity);
    const double tcalc = (deltapos.Dot(deltapos));

    const std::complex<double> *solutions = solve_quartic(t3_calc / t4_calc, t2_calc / t4_calc, t1_calc / t4_calc, tcalc / t4_calc); //quatric gleichung, berechnet auch targetacceleration des Ziels mit (Aber nicht targetacceleration der Rakete)

    double solutions0 = solutions[0].real();
    double solutions1 = solutions[1].real();
    double solutions2 = solutions[2].real();
    double solutions3 = solutions[3].real();
    double solutions4 = solutions[4].real();

    double deltasolution = solutions->real();
    if (deltasolution < 0) {
        deltasolution *= -1;
    }

    return deltasolution;
}

Vector3 freezetarget;
bool painted = false;

//unlike csgo, many realistic games like arma 3 have a bullet drop and bullet speed. that means you can't just aim the crosshair at the enemy to hit, you have to aim slightly ahead and over the enemy based on how far away the enemy is, the enemys velocity and acceleration, for the bullet to hit. This function calculates exactly that, how far the aimbot has to aim above and in front of the target in order to hit.
Vector3 Aimbot::predict_impact_pos(const Vector3 &source, const Vector3 &destination, const Vector3 &target_velocity, Vector3 &impact_coords, double &hittimetokill, double &hitvelocity,double &hitdistance) //main aimbot prediction calculation, s
{
    double timetoimpact = 0;
    double bullettime = 0;
    Vector3 predict_impact_coords = destination;

    //-------------------------------------------------------------------

    const Vector3 targetacceleration = getacceleration(target_velocity); //calculate the acceleration of the target - maybe useless against infrantry but great against fast accelerating vehicles and planes - if you use this only for infrantry you can disable this and replace it with {0,0,0} instead
    double distance = source.DistTo(destination); //Distance to target in meters

    //-------------------------------------------------------------------

    //Since in many games the bullet does not have a constant speed because of air friction, i.e. it slows down with time and the bullet drop gets higher, the following function is necessary to calculate the time / speed to the target.
    if (targetacceleration.DistTo({0, 0, 0}) <= 0.25 && targetacceleration.DistTo({0, 0, 0}) >= -0.5) //If the target has no or only a very small acceleration then we use a quadratic equation for the time and position calculation without using the acceleration
    {
        Vector3 velocity;
        if (target_velocity.DistTo({0, 0, 0}) <= 0.25 && target_velocity.DistTo({0, 0, 0}) >= -0.25) //many gaames are weird... even when infrantry, and vehicles are not moving they sometimes still have a small fluctuating velocity which confuses the calculation. If the velocity is below 0.25 you can assume that the target is not moving, then we just ignore the velocity completely.
        {
            velocity = Vector3(0, 0, 0);
        } else {
            velocity = target_velocity;
        }

        //-------------------------------------------------------------------

        //Some of you who know a bit of aimbot coding probably thinking, why is he using this overly complicated function to calculate the time to target? 
        //Most of you would probably just do "distance / bulletspeed = time" and yes that works fine as long as the target is not moving nearer or further away from you. But we also have to calculate moving Targets, so that wouldn't be precise.
        //And here is why, as an example, your bullet is flying at 100 meters a second, the target is 1000 meters away and is moving away from you at a speed of 10 meters per second. 
        //If you now calculate Distance / Bulletspeed = Time, then you would get 10 Seconds. That means the bullet would need 10 seconds to reach the target which means your Aimbot will shoot at the position where the opponent should be in 10 seconds. 
        //But since the target moves away from you at 10 meters per second, after the 10 seconds, when the bullet should reach the target, the target is no longer 1000 meters away, but 1100 meters. 
        //So the bullet would actually need 10.1 seconds from the moment you pull the trigger until it reaches the target instead of 10 seconds.
        //Even if it is only a tenth of a second, this tenth can mean a hit or a miss cause in that .1 second the target would move 1 more meter, especially this would fail on Vehicles and Jets that can be way faster than 10 M/s.
        //And this is exactly what is calculated in the function below. At what position the target will be based on velocity, and how long the bullet will take to get to that position.
        
        //We could also account that the Bullet slows down over time because of Drag but we won't calculate that here since it would get too complicated with all of that

        timetoimpact = quadratic_get_time_to_target(destination, source, bulletspeed, velocity, (distance / bulletspeed)); //calculate how many seconds the bullet will travel until it hits the target beased on target velocity (without target acceleration)

        predict_impact_coords = calculate_impact_position(timetoimpact, destination, velocity, { 0,0,0 }); //calculates the xyz position at which the bullet and the target meet and hit (without target targetacceleration)

        //-------------------------------------------------------------------

        //for (int i = 0; i <= 5; i++) { //recalculate to make it more precise, this can be removed if it is already precise enough for you.
        //    distance = source.DistTo(predict_impact_coords); //Distance to previous calculated impact position in Meters
        //    bullettime = distance / bulletspeed;
        //    predict_impact_coords = calculate_impact_position(bullettime, destination, velocity, { 0,0,0 }); //Berechnet die Position bei der sich die Rakete und das Ziel treffen (Ohne Target targetacceleration)
        //}

    } 
    else //Otherwise if the target is a accelerating or deaccelerating, we use a quatric equation for calculating the time to impact and the impact position
    {

        Vector3 velocity;
        if (target_velocity.DistTo({0, 0, 0}) <= 0.25 && target_velocity.DistTo({0, 0, 0}) >= -0.25) 
        {
            velocity = Vector3(0, 0, 0);
        } else {
            velocity = target_velocity;
        }

        //-------------------------------------------------------------------

        timetoimpact = quatric_get_time_to_target(destination, source, bulletspeed, velocity, targetacceleration);

        predict_impact_coords = calculate_impact_position(timetoimpact, destination, velocity, targetacceleration);

        //-------------------------------------------------------------------

        //for (int i = 0; i <= 5; i++) {
        //    distance = source.DistTo(predict_impact_coords);
        //    bullettime = distance / bulletspeed;
        //    predict_impact_coords = calculate_impact_position(bullettime, destination, velocity, targetacceleration);
        //}
    }


    if (GetAsyncKeyState(VK_LBUTTON)) { //if left button is pressed, draw prediction
        freezetarget = predict_impact_coords;
        painted = true;
    }

    //----------------------------Overlay---------------------------------------
    
    impact_coords = predict_impact_coords;
    hittimetokill = timetoimpact;
    hitvelocity = targetacceleration.DistTo({0, 0, 0});
    hitdistance = predict_impact_coords.DistTo(source);

    //---------------------------------Account Gravity----------------------------------

    const double predictdistance = source.DistTo(predict_impact_coords);
    float tt = predictdistance / distance;

    float bulletDrop = 0.5f * 9.81 * pow(tt, 2); //Calculates how far you have to aim above the target to compensate for gravity based on the bullet travel time (might be inaccurate for different games, then you can use 3d curve fitting instead).
    predict_impact_coords.y += bulletDrop;
   
    //-------------------------------------------------------------------

    return predict_impact_coords; //Returns the position where you have to aim to hit the Target
}

//------------------------------------------------------------------------------------------------------------------------

Vector3 to_predicted_pos;
Vector3 to_direct;
Vector3 drawpos;
Vector3 to_draw;
Vector3 to_posdraw;

TargetInfo Aimbot::get_target_info(const uint64_t entity) {

    if (const bool dead = read<bool>(entity + 0x00); dead == true) //If Target is dead then ignore it (If you don't have a dead / alife check just remove this)
    {
        return {};
    }

    //-------------------------------------------------------------------

    const auto myposition = initialize_offsets::local_player_position();    //Get your own Player Position

    const auto preposptr = read<uint64_t>(entity + 0x190);          //Pointer for Entity
    const auto targetposition = read<Vector3>(preposptr + 0x2C);   //Entity Position
    const auto entity_velocity = read<Vector3>(preposptr + 0x54);   //Entity Velocity

    //-------------------------------------------------------------------

    if ( targetposition.x == NULL || (targetposition.y == 0 && targetposition.x == 0 && targetposition.z == 0)) {   //Check if Entity Position is NULL or 0 (If so then return)
        return {};
    }

    //-------------------------------------------------------------------

    if (myposition.DistTo(targetposition) <= 5) {  //Check if the Entity is nearer than 5 meters, if so then disable ESP (Doing this ghetto method to disable ESP on your own Player)
        return {};
    }

    //-------------------------------------------------------------------

    Vector3 nothing;
    double hittimetokill;
    double hitvelocity;
    double hitdistance;

    const Vector3 aim_prediction = predict_impact_pos(myposition, targetposition, entity_velocity, nothing, hittimetokill, hitvelocity, hitdistance); //Gives the XYZ Position where the Bullet will hit the Target


    if (aim_prediction.x == 0)
        return {};

    TargetInfo target_info;

    to_predicted_pos = c_graphics_instance::world_to_screen(aim_prediction);
    to_direct = c_graphics_instance::world_to_screen(targetposition);

    if (to_predicted_pos.x != 0 && to_predicted_pos.y != 0) {
        target_info.velocity = entity_velocity;
        target_info.screenPosition = to_predicted_pos;
        target_info.screenPositionNoPredict = to_direct;
        target_info.entity = entity;
        target_info.isInScreen = true;
        target_info.Position = targetposition;
    }
    else {
        target_info.entity = 0;
        target_info.screenPosition = Vector3(0, 0, 0);
        target_info.isInScreen = false;
    }

    return target_info;
}

D2D1::ColorF aimcol = D2D1::ColorF(0.0f, 0.0f, 1.0f, 1.0f);
D2D1::ColorF impactcol = D2D1::ColorF(1.0f, 0.0f, 0.0f, 0.5f);

class vec2_xx {
public:
    float x;
    float y;

    vec2_xx();

    vec2_xx(float a, float b);
};

vec2_xx::vec2_xx() {
    x = 0;
    y = 0;
}

vec2_xx::vec2_xx(float a, float b) {
    x = a;
    y = b;
}

int countnotvisiblex = 0;

bool firsty = true;

bool unlockent = true;

void Aimbot::on_update() {


    if (painted) //Paints the Circle, where the Bullet will hit the Target if you click left Mouse
    {
        const Vector3 to_freezetrget = c_graphics_instance::world_to_screen(freezetarget);
        //const Vector3 to_freezetrget2 = c_graphics_instance::world_to_screen({ freezetarget.x, 0, freezetarget.z });
        FOverlay::draw_circle(to_freezetrget.x, to_freezetrget.y, 25, 5, &impactcol);
        //FOverlay::draw_line(to_freezetrget2.x, to_freezetrget2.y, to_freezetrget.x, to_freezetrget.y, &impactcol);
        FOverlay::draw_text(to_freezetrget.x - 22, to_freezetrget.y + 30, &impactcol, "IMPACT");

        if (!GetAsyncKeyState(VK_CAPITAL)) {
            painted = false;
        } else {
            return;
        }
    }

    if (!is_lock && !GetAsyncKeyState(VK_CAPITAL)) { //Enable aimbot on VK_Capital
        current_target = TargetInfo();
        unlockent = true;
        return;
    }

    if (GetAsyncKeyState(VK_MBUTTON)) { //Lock and unlock aimbot on target x
        is_lock = !is_lock;
        Sleep(300);
    }

    if (unlockent || current_target.entity == 0) {
        unlockent = false;
        current_target = get_best_target();
    }

    current_target = get_target_info(current_target.entity);

    if (current_target.isInScreen) {
        if (current_target.entity != 0) {
            //cout << time_span.count() << " seconds\n";

            InputSys::Get().move_mouse(
                    (static_cast<int>(current_target.screenPosition.x) - SCREEN_WIDTH / 2) * aimbot_speed,
                    (static_cast<int>(current_target.screenPosition.y) - SCREEN_HEIGHT / 2) * aimbot_speed);

            //FOverlay::draw_circle(static_cast<int>(current_target.screenPosition.x), static_cast<int>(current_target.screenPosition.y) + 250, 3, 1, &aimcol);
        }
    }
}

uint64_t thetargetx;

void Aimbot::initialize() {
    on_update();
}
