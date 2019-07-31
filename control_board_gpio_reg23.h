// control_board_gpio_reg23.h

#include <cstdint>      //  std::uint16_t
#include <exception>    //  std::range_error
#include <sstream>      //  std::stringstream


// in real life there we can expect multiple GPIO registers. In this toy
// example we happen to be working with GPIO register #23

// bit fields for the our toy "register"
struct genpurpIO_register23
{
     std::uint16_t energize_vac_solenoid2 : 1;    // set to 1 to apply vacuum
     std::uint16_t energize_vac_solenoid3 : 1;    // set to 1 to apply vacuum
     std::uint16_t lamp_pwr               : 3;    // 0 == lights off; 7 == max illumination
     std::uint16_t                        : 11;   // fill to 16 bits
};

typedef struct genpurpIO_register23* gpio_reg23_ptr_t;

enum class vacuum: unsigned int
{
    OFF,  // de-energizing the vacuum solenoid closes the valve, removing the vacuum
    ON    //    energizing the vacuum solenoid opens  the valve, applying the vacuum
};

//  lamp_t range:  0:7
//                       0 == lights out
//                       7 == max illumination
//
//  Note1:  The values of these named constants are selected
//          so as to faciliate the 'walking ones' testing to
//          be conducted during unit testing
//
//          Also, assume the lamp's technology enables it
//          to generate illumination exactly proportional
//          to power applied (that is, this is not an
//          incandescent bulb.)
//
const std::uint16_t     LAMP_OOR          = 8; // lamp's Out Of Range value
const std::uint16_t     FULL_ILLUMINATION = 7;
const std::uint16_t     BRIGHT_LIGHTS     = 4; // Note1
const std::uint16_t     MOOD_LIGHTING     = 2; // Note1
const std::uint16_t     VERY_DIM_LIGHTS   = 1; // Note1
const std::uint16_t     LIGHTS_OUT        = 0;

//-------- These typedefs only exist to instantiate partial specializations ----------

typedef struct solenoid2 * solenoid2_t;
typedef struct solenoid3 * solenoid3_t;
typedef std::uint16_t      lamp_t;

//-------- end of partial specialization typedefs ----------

// Note2:   no need to define gpio_register_23 b/c the
//          primary template is never instantiated.

// primary template
template< typename field>
class gpio_register_23;    // Note2

// class template partial specialization
// for the vac_solenoid2 control functor
template<>
class gpio_register_23< solenoid2_t  >
{
public:
    gpio_register_23(gpio_reg23_ptr_t preg_)  : preg(preg_)
    {
        preg->energize_vac_solenoid2 = 0;  // close the valve on startup
    }

    // functor for controlling the vacuum solenoid
    // returns the solenoid's previous state.
    vacuum operator() (vacuum val)
    {
        // store the solenoid's current state
        vacuum retval = get_current_state();

        // set solenoid to new state
        preg->energize_vac_solenoid2 = (val == vacuum::OFF ? 0 : 1);

        // return the solenoid's 'prior to call' state
        return retval;
    }

    // functor for returning the vacuum solenoid's current state
    vacuum operator() ()
    {
        return get_current_state();
    }

private:
    vacuum get_current_state()
    {
        // init to vacuum solenoid being de-energized
        vacuum retval = vacuum::OFF;

        // if power is currently applied to the vacuum solenoid
        if (preg->energize_vac_solenoid2 == 1)
        {
            retval = vacuum::ON;
        }

        return retval;
    }

    volatile gpio_reg23_ptr_t preg;
};



// class template partial specialization
// for the vac_solenoid3 control functor
template<>
class gpio_register_23< solenoid3_t >
{
public:
    gpio_register_23(gpio_reg23_ptr_t preg_)  : preg(preg_)
    {
        preg->energize_vac_solenoid3 = 0;  // close the valve on startup
    }

    // functor for controlling the vacuum solenoid
    // returns the solenoid's previous state.
    vacuum operator() (vacuum val)
    {
        // store the solenoid's current state
        vacuum retval = get_current_state();

        // set solenoid to new state
        preg->energize_vac_solenoid3 = (val == vacuum::OFF ? 0 : 1);

        // return the solenoid's 'prior to call' state
        return retval;
    }

    // functor for returning the vacuum solenoid's current state
    vacuum operator() ()
    {
        return get_current_state();
    }

private:
    vacuum get_current_state()
    {
        // init to vacuum solenoid being de-energized
        vacuum retval = vacuum::OFF;

        // if power is currently applied to the vacuum solenoid
        if (preg->energize_vac_solenoid3 == 1)
        {
            retval = vacuum::ON;
        }

        return retval;
    }

    volatile gpio_reg23_ptr_t preg;
};

// class template partial specialization
// for the lamp control functor
template<>
class gpio_register_23< lamp_t >
{
public:
    gpio_register_23(gpio_reg23_ptr_t preg_)  : preg(preg_)
    {
        preg->lamp_pwr = LIGHTS_OUT;  // kill the lamp on startup
    }

    // functor for controlling the lamp's power setting
    std::uint16_t operator() (lamp_t val)
    {
        // if the caller specified a power setting that is out of range
        if (val >= LAMP_OOR)
        {
            std::stringstream msg{};
            msg << "Incorrect attempt to set lamp #42 pwr value to (" << val << "). "
                << "Valid pwr settings range for lamp #42 is 0:7. ";
            throw std::range_error( msg.str() );
        }

        // store the lamp's current power setting
        std::uint16_t retval = get_current_state();

        preg->lamp_pwr = {val};     // update lamp power setting

        // return the lamp's 'prior to call' power setting
        return retval;
    }

    // functor for returning the lamp's current power setting
    std::uint16_t operator() ()
    {
        return get_current_state();
    }


private:
    std::uint16_t get_current_state()
    {
        return preg->lamp_pwr;
    }

    volatile gpio_reg23_ptr_t preg;
};



