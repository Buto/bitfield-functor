// bitfield.h

#include <cstdint>      //  std::uint16_t
#include <exception>    //  std::range_error

// My motivations for conducting this exercise was to
// experiment in a new way for writing abstractions manipulating
// bits in a register, say twiddling the bits in a GPIO register.
//
// Normally this would just mean abstracting this code into
// one or more functions, creating some kind of readable
// constants that are to be used as arguments to said functions.
// This time I wanted to abstract such code into functors instead
// of functions.  At the call site both functions and functors
// would appear indistinguishable from each other.
//
// In addition, this time I wanted to use bit-fields instead of
// the "raw bits" approach (i.e., bitwise ANDing,ORing and so on.)
// NB: I've always understood bit-fields perfectly well, it's just
// that everyone else almost always chose to go with the
// "raw bits" technique and when you're part of a team you shouldn't
// buck the consensus without a damn good reason. That said, I've
// always considered bit-fields to be a more natural looking way to
// twiddle bits in a register.
//
// So, in short, I had the following objectives:
//
//      1)  I wanted to work with functors
//      2)  I wanted to work with partial template specialization
//      3)  I wanted to try using bit-fields instead of the "raw bits" technique
//      4)  I wanted to discover how much extra code is required to go the
//          generic programming template route.
//
// --------------------------------------------------------------------
//
// For this exercise I am creating functors for setting a specific
// bit field in a register.  This register maps to 16 bits of
// general purpose I/O (GPIO.)
//
// This GPIO register controls:
//      1) two solenoids, each of which operate a valve on a vaccum line; and
//      2) a floodlamp's brightness.
//
// Assume the necessary exteneral circuitry is implemented. For example:
//      1)  Assume that the bits associated with controlling the
//          vaccum solenoids are connected to circuity (relay drivers,
//          most likely) suitable for controlling power to a
//          solenoid's coil.
//
//      2)  Assume that the bits associated with controlling the
//          flood lights are connected to a DAC, followed by
//          power control circuitry suitable for modulating the
//          power provided to the floodlight.
//
// A distinct functor is provided to each field of bits in the register.
//
// commentary:
//
// About portabilty:
//      The layout of the field within a bitfield is implementation dependent.
//      Consequently, this code cannot be expected to be portable between
//      C++ compilers.
//
//      This is probably OK for the purposes of embedded programming b/c
//      in the varous shops in which I worked portablity wasn't a concern:
//      since each shop used only used one particular C++ compiler for the
//      given microcontroller they were using.
//
// About simplicity of interface WRT complexity of implementation
//
//      Simplicity of the interface is more valuable than simplicity of
//      the implementation.  The implementation can be made somewhat
//      more complex so as to simplify the interface.
//
//      Just using bit fields instead of bitwise ANDing and ORing values
//      does more to simplify the implementation than either functors or
//      using a class did to complicate the implementation.
//
//      I score the interface created by functors as slightly less complex than
//      the interface implyed by a class.
//
// Annoyances:
//
//      1)  The point of templates is to facilitate 'generic programming'.
//          Unfortunately, the highly hardware implementation specific
//          concerns of register bit-twiddling defeats generic programming's
//          intent of code reuse.
//
//          That said, an exception to this would be a case where the
//          system includes multiple registers where these registers
//          all have the same bit layouts. In this case you actually
//          could reuse the same template to create a an additional
//          set of functors for each register with identical bit layouts.
//          One scenario for how a system could have two different
//          registers each with identical layouts is the NEC8250, a
//          dual UART IC. Each UART in this IC has their own set of
//          registers, which means each register is duplicated.
//          (Credit to Ben Adams for reminding me of the dual UART
//          scenario.)
//
//          Note that in this example some of the error messages contain,
//          say, the floodlamp ID. Some kind of accomodation for passing an
//          ID will be needed before this template can be reused for
//          different GPIO registers because hardcoding an ID into an
//          error message makes the template unsuitable for reuse.
//
//      2)  Each field within the bit-field requires its own partial specialization of
//          the class template, even if there are two fields with exactly the
//          same usage (scenario: each of the two vacuum solenoids had a distinct field.)
//          This annoyed because it meant to code duplication, which defeats the
//          recommendation to keep your code DRY.
//
//      3)  I needed to introduce extra enumerations just to create distinct
//          a partial specialization for each solenoid. This annoyed because
//          I had to create additional, otherwise pointless enumerations just
//          because of the way partial specialization works.
//
//          Explanation: By definition, each partial specialization must
//          have a distinct arguement list WRT the primary template for
//          which it is providing a specialization.
//
//          The code for these two solenoids each require their own
//          partial specialization of the class template. Both of these
//          partial specializations are the same except for the bit field
//          they manage.
//
//          I chose to create a distinct enumeration for each of the two
//          partial specialization of the class template that were dedicated
//          to managing solenoids.
//
//  Tradeoffs: creating a partial specialization of a class template for
//             each field vs creating a class with a method for each field.
//
//      1)  Implement this via, say, a class would not help the code duplication
//          concern because the implementation of the methods for the vacuum
//          solenoids would still mean two distinct member functions, one for
//          each vacuum solenoid. (wash)
//
//      2)  Implement this via, say, a class would remove the need to create a
//          enumerations just for the purpose of creating a distinct
//          partial specialization for each solenoid.  (class wins)
//
//      3) Functors do not require instatating an instance of the class.
//          (functors win)
//
//      3) The functor() interface for setting the bit field is slightly
//          less complex. Put differently, calling a functors appears
//          slightly cleaner than calling a class' member function.
//          For example:
//              obj.vac_solenoid2(vacuum::ON);
//          vs
//              vac_solenoid2(vacuum::ON);
//
//          Ironically the advantage of functors is that a functor call
//          appears to be indistinguishable from calling a C function.
//          (And what appears simpler than a C function call?)
//
//          This example's vacuum solenoid functors have the additional
//          advantage of being typesafe. For example it is impossible to
//          call vac_solenoid2() with an out of range value
//          (scenario: vac_solenoid2(3) ) because passing an integer to
//          this functor will cause an error at compile time.
//          (functors win)
//
//      4)  The functor() interface for 'reading back' the bit field's
//          current is akin to an overloaded method in that:
//              a) the signature is different, and
//              b) the return types are different.
//
//          For example, using the 'read back' functor to obtain the
//          state of vacuum solenoid #2 is just:
//
//              vacuum scratch = vac_solenoid2();
//
//          Of course you can obtain the same behavior with a
//          standard class by overloading a method.
//
//      Conclusion: I cannot, with a straight face, make an argument that
//                  either approach has a particularly strong advantage.
//                  Both seem tolerable to me. I think I like the functors
//                  better.
//
// About practicality:
//
//      Each group of bits in a bit field is called a 'field'.
//      In this toy application the two named fields are:
//          1) energize_vac_solenoid
//          2) floodlight_pwr
//      The solution I found required a partial specialization class template for
//      each field.
//
//      Each partial specialization defines a different functor.
//      Each functor manages a distinct field in the bit field.
//
//      The advantage of this scheme is the functor is intialized (RAII) with
//      the address of the 'register'. Afterward the functor's sole parameter is
//      value used to update the field managed by the given functor.  Also
//      error handling is easier to implement in a function than in a macro.
//
//      The cost of this scheme is that we have to write a partial
//      specialization class template for each field in the register.
//      The tradeoff is that it is more convenient to provide greater
//      functionality to functors than to macros.
//
//      Another cost is that we have to create a distinct type for each bitfield.
//      For example, the toy "register" has one bit for controlling a
//
// this toy application has a type named solenoid2_t, which creates
//      a functor for a particular bit field.  If the toy register
//
// About a possible extension
//      Arguably, the functors could be extended such that they could also store
//      a string with a name to be associated with a given field. For example,
//      in a system where you have solenoids named SOL1, SOL2, and SOL3 their
//      respective functor could be initialized with these names, which could
//      then be used to augment exception error messages.
//
//=============================================================================

// bit fields for the our toy "register"
struct genpurpIO_register
{
     std::uint16_t energize_vac_solenoid2 : 1;    // set to 1 to apply vacuum
     std::uint16_t energize_vac_solenoid3 : 1;    // set to 1 to apply vacuum
     std::uint16_t floodlight_pwr         : 3;    // 0 == lights off; 7 == max illumination
     std::uint16_t                        : 11;   // fill to 16 bits
};

enum class vacuum: unsigned int
{
    OFF,  // de-energizing the vacuum solenoid closes the valve, removing the vacuum
    ON    //    energizing the vacuum solenoid opens  the valve, applying the vacuum
};

//  floodlight_t range:  0:7
//                       0 == lights out
//                       7 == max illumination
//
//  Note1:  The values of these named constants are selected
//          so as to faciliate the 'walking ones' testing to
//          be conducted during unit testing
//
const std::uint16_t     FLOODLIGHT_OOR    = 8; // Floodlight's Out Of Range value
const std::uint16_t     FULL_ILLUMINATION = 7;
const std::uint16_t     BRIGHT_LIGHTS     = 4; // Note1
const std::uint16_t     MOOD_LIGHTING     = 2; // Note1
const std::uint16_t     VERY_DIM_LIGHTS   = 1; // Note1
const std::uint16_t     LIGHTS_OUT        = 0;

//-------- These typedefs only exist to instantiate partial specializations ----------
typedef enum vac_solenoid2_e
{
    solenoid2_OOR  = 2
} solenoid2_t;

typedef enum vac_solenoid3_e
{
    solenoid3_OOR  = 2
} solenoid3_t;

typedef std::uint16_t   floodlight_t;

//-------- end of partial specialization typedefs ----------

// primary template
template< typename bf, typename field, typename value >
class set_bits { };

// class template partial specialization
// for the vac_solenoid2 control functor
template< typename bf>
class set_bits< bf, solenoid2_t, vacuum  >
{
public:
    set_bits( bf fld ) : reg{fld} // ctor saves register's address
    {
        reg->energize_vac_solenoid2 = 0;  // close the valve on startup
    }

    // functor for controlling the vacuum solenoid
    // returns the solenoid's previous state.
    vacuum operator() (vacuum val)
    {
        // store the solenoid's 'prior to call' state
        vacuum retval = (reg->energize_vac_solenoid2 == 0 ? vacuum::OFF : vacuum::ON );

        // set solenoid to new state
        reg->energize_vac_solenoid2 = (val == vacuum::OFF ? 0 : 1);

        // return the solenoid's 'prior to call' state
        return retval;
    }

    // functor for returning the vacuum solenoid's state
    vacuum operator() ()
    {
        // init to vacuum solenoid is currently de-energized
        vacuum retval = vacuum::OFF;

        // if power is currently applied to the vacuum solenoid
        if (reg->energize_vac_solenoid2 == 1)
        {
            retval = vacuum::ON;
        }

        return retval;
    }

private:
    volatile bf  reg;
};


// class template partial specialization
// for the vac_solenoid3 control functor
template< typename bf>
class set_bits< bf, solenoid3_t, vacuum >
{
public:
    set_bits( bf fld ) : reg{fld} // ctor saves register's address
    {
        reg->energize_vac_solenoid3 = 0;  // close the valve on startup
    }

    // functor for controlling the vacuum solenoid
    // returns the solenoid's previous state.
    vacuum operator() (vacuum val)
    {
        // store the solenoid's 'prior to call' state
        vacuum retval = (reg->energize_vac_solenoid3 == 0 ? vacuum::OFF : vacuum::ON );

        // set solenoid to new state
        reg->energize_vac_solenoid3 = (val == vacuum::OFF ? 0 : 1);

        // return the solenoid's 'prior to call' state
        return retval;
    }

    // functor for returning the vacuum solenoid's state
    vacuum operator() ()
    {
        // init to vacuum solenoid is currently de-energized
        vacuum retval = vacuum::OFF;

        // if power is currently applied to the vacuum solenoid
        if (reg->energize_vac_solenoid3 == 1)
        {
            retval = vacuum::ON;
        }

        return retval;
    }

private:
    volatile bf  reg;
};

// class template partial specialization
// for the floodlight control functor
template< typename bf>
class set_bits< bf, floodlight_t, floodlight_t >
{
public:
    set_bits( bf fld ) : reg{fld} // ctor saves register's address
    {
        reg->floodlight_pwr = LIGHTS_OUT;  // kill the floodlamp on startup
    }

    // functor for controlling the floodlamp's power setting
    std::uint16_t operator() (floodlight_t val)
    {
        // if out of range
        if (val >= FLOODLIGHT_OOR)
        {
            std::stringstream msg{};
            msg << "Incorrect attempt to set floodlight #42 pwr value to (" << val << "). "
                << "Valid pwr settings range for floodlight #42 is 0:7. ";
            throw std::range_error( msg.str() );
        }

        // store the floodlamp's 'prior to call' power setting
        std::uint16_t retval = reg->floodlight_pwr;

        reg->floodlight_pwr = {val};     // update floodlight power setting

        // return the floodlamp's 'prior to call' power setting
        return retval;
    }

    // functor for returning the floodlamp's power setting
    std::uint16_t operator() ()
    {
        return reg->floodlight_pwr;
    }


private:
    volatile bf  reg;
};


