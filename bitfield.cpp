// bitfield.cpp

#include <algorithm>    // find_if
#include <cassert>      //  assert
#include <cstdint>      //  std::uint16_t
#include <exception>    //  std::range_error
#include <iostream>     //  for debugging to stdout
#include <string>       //  for std::string
#include <stdlib.h>     //  exit(), EXIT_FAILURE
#include <sstream>      //  std::stringstream

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
//
// Stuff that doesn't work
//
// example1:
//
// typedef enum vac_solenoid_e
// {
//     vacuum_off = 0,  // vacuum solenoid de-energized
//     vacuum_on  = 1,  // vacuum solenoid energized
//     solenoid_OOR     // Out Of Range
// } solenoid_t;
//
// typedef solenoid_t  solenoid2_t;  // this isn't going to work
// typedef solenoid_t  solenoid3_t;  // this isn't going to work
//
// class template partial specialization
// for the vac_solenoid2 control functor
//template< typename bf >
//class set_bits< bf, solenoid2_t >
//{ ... };
//
//template< typename bf >
//class set_bits< bf, solenoid3_t >   // ERROR:  error: redefinition of ‘class set_bits<bf, vac_solenoid_e>’
//{ ... };
//
// analysis of why example1 doesn't work:
//      Evidently the C++ "collapses" a sequences of typedefs,say, in the form of:
//     typedef enum foo_e {...} foo_t;
//     typedef enum foo_t foo1_t;
//     typedef enum foo_t foo2_t;
// such that both foo1_t and foo2_t resolve back to foo_t.
// This causes the attempt to create a partial template specialization for class set_bits< bf, solenoid3_t > to fail.
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
const std::uint16_t     FULL_ILLUMINATION = 7; // Note1
const std::uint16_t     BRIGHT_LIGHTS     = 4; // Note1
const std::uint16_t     MOOD_LIGHTING     = 2; // Note1
const std::uint16_t     VERY_DIM_LIGHTS   = 1; // Note1
const std::uint16_t     LIGHTS_OUT        = 0; // Note1

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

// ============ helper debug functions ================================
void print_vac_state( std::uint16_t val)
{
    switch ( val )
    {
        case 0: std::cout << "de-energized" <<  ")" << std::endl; break;
        case 1: std::cout << "energized"    <<  ")" << std::endl; break;
        default:
            {
                std::cout   << "solenoid bitfield contains impossible value  (" << val << "). "
                            << std::endl;
                assert (!"solenoid bitfield contains impossible value");
            }
        break;
    }
}

//-----------------------------------------------------
// print_reg() --   prints state of the 'register' to stdout
//
//                  For this function to work correctly in
//                  real life the 'register' will need to
//                  be R/W, not just write-only

void print_reg(struct genpurpIO_register* reg)
{
    std::cout << "reg value (raw bits)" << reg->energize_vac_solenoid2  << std::endl;
    std::cout << "reg value (raw bits)" << reg->energize_vac_solenoid3  << std::endl;
    std::cout << "reg value (raw bits)" << reg->floodlight_pwr  << std::endl;

    std::cout << "vac_solenoid2 state:(";
    print_vac_state( reg->energize_vac_solenoid2 );

    std::cout << "vac_solenoid3 state:(";
    print_vac_state( reg->energize_vac_solenoid3 );

    std::cout << "flood light pwr setting:(" << reg->floodlight_pwr << ")" << std::endl << std::endl;
}

// ============ end of helper debug functions ================================

const std::size_t ok_col_pos   { 95 };  // column position for "ok" text   See note1

// rtrim() -- trim trailing dots from end of a string
//
// credits: https://stackoverflow.com/a/217605
static inline void rtrim(std::string &s)
{
    s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch)
            {
                return ch != '.';
            }
        ).base(), s.end()
    );
}


// note1:   for readablity I favor formatting UT chatter
//          in text columns. Generally the first column is
//          a description of what the given UT is testing and
//          the second column shows that the test passed (i.e., "ok").
//
//          The text emitted when a UT fails is formatted differently
//          WRT chatter from a successful UT. Put another way,
//          a UT failure emits text that visually clashes with
//          the text generated by successful UTs.
//
//          The human eyes is a mismatch detector.  This formatting
//          scheme intents to make a stream of successful UTs
//          "blend together" such that any UT failure will be
//          instantly noticed because it visually clashes with
//          chatter from successful UTs.
//
//  note4:  As per note1, we are inserting the "ok" text in a
//          second 'column' on the line.  We're using insert() to
//          create a "tab stop" effect where the second 'column' is
//          located.
//
//          This column's location follows the chatter about what the
//          given UT is testing. This "tab stop" is located beyond the
//          end of the chatter about what a given UT is testing.
//
//          string.insert(pos, text) will throw out_of_range if
//          pos > string length.
//
//          Consequently, we are padding the end of the first column's
//          chatter with whitespace so as to prevent string.insert()
//          from throwing "out_of_range" when we insert "ok" at
//          the second column's "tab stop".


// ut_verify_solenoid_state() general purpose UT boilerplate for
// unit testing the functor that returns the current state of a solenoid
//
// keeping it DRY
int ut_verify_solenoid_state(
                                const std::string& utid,        // ut17, ut18, etc.
                                const std::string& intent,      // what UT is attempting to verify
                                const vacuum actual_state,      // solenoid's actual state
                                const vacuum expected_state     // solenoid's expected state
                            )
{
    int something_failed = 1;    // init to UT failure
    std::stringstream ut_intent {};

    const std::string str_actual_state   { (actual_state   == vacuum::ON ? "vacuum::ON" : "vacuum::OFF") };
    const std::string str_expected_state { (expected_state == vacuum::ON ? "vacuum::ON" : "vacuum::OFF") };

    ut_intent <<  intent;

    std::cout << utid << ": ";

    // if the code worked as expected
    if (actual_state == expected_state)
    {
        something_failed = 0; // indicate UT passed
    }

    if (something_failed)
    {
        std::cout << "FAILED!" << std::endl;
        std::cout << ut_intent.str() << std::endl;
        std::cout << "expected("    << str_expected_state << ")" << std::endl;
        std::cout << "encountered(" << str_actual_state   << ")" << std::endl;
    }
    else
    {
        //  format "UT Passed" chatter. See note1

        ut_intent <<  "......................................................";   // note4
        std::string tmp { ut_intent.str()  };
        tmp.insert( ok_col_pos, "ok" );     // columnize "ok" text   See note1
        rtrim(tmp);                         // toss trailing dots
        std::cout << tmp << std::endl;
    }

    return something_failed;
}


// ut_verify_lamp_state() general purpose UT boilerplate for
// unit testing the functor that returns the current state of floodlamp
//
// keeping it DRY
int ut_verify_lamp_state(
                            const std::string& utid,               // ut17, ut18, etc.
                            const std::string& intent,             // what UT is attempting to verify
                            const std::uint16_t actual_settings,   // flood lamp's actual state
                            const std::uint16_t expected_settings  // flood lamp's expected state
                        )
{
    int something_failed = 1;    // init to UT failure
    std::stringstream ut_intent {};

    ut_intent <<  intent;

    std::cout << utid << ": ";

    // if the code worked as expected
    if (actual_settings == expected_settings)
    {
        something_failed = 0; // indicate UT passed
    }

    if (something_failed)
    {
        std::cout << "FAILED!" << std::endl;
        std::cout << ut_intent.str() << std::endl;
        std::cout << "expected("    << actual_settings << ")" << std::endl;
        std::cout << "encountered(" << expected_settings   << ")" << std::endl;
    }
    else
    {
        //  format "UT Passed" chatter. See note1

        ut_intent <<  "......................................................";   // note4
        std::string tmp { ut_intent.str()  };
        tmp.insert( ok_col_pos, "ok" );     // columnize "ok" text   See note1
        rtrim(tmp);                         // toss trailing dots
        std::cout << tmp << std::endl;
    }

    return something_failed;
}

//======================= Unit Tests Begin ======================================
//
// verify that the ctor set solenoid2 to vacuum:OFF
int ut00()
{
    static struct genpurpIO_register mock_reg84;   // This is masquerading as a 16 bit "register"

    set_bits< struct genpurpIO_register*, solenoid2_t, vacuum   > vac_solenoid2{ &mock_reg84 }; // for vacuum solenoid #2

    std::string ut_intent {  "verifing that the ctor initialized solenoid2 to vacuum:OFF" };

    return ut_verify_solenoid_state(
                                        std::string { __func__ }, // utid,
                                        ut_intent,                // what UT is attempting to verify
                                        vac_solenoid2(),          // solenoid's actual state
                                        vacuum::OFF               // solenoid's expected state
                                   );
}

// verify that the ctor set solenoid3 to vacuum:OFF
int ut01()
{
    static struct genpurpIO_register mock_reg84;   // This is masquerading as a 16 bit "register"

    set_bits< struct genpurpIO_register*, solenoid3_t, vacuum   > vac_solenoid3{ &mock_reg84 }; // for vacuum solenoid #2

    std::string ut_intent {  "verifing that the ctor initialized solenoid3 to vacuum:OFF" };

    return ut_verify_solenoid_state(
                                        std::string { __func__ }, // utid,
                                        ut_intent,                // what UT is attempting to verify
                                        vac_solenoid3(),          // solenoid's actual state
                                        vacuum::OFF               // solenoid's expected state
                                   );
}


// verify that the ctor sets the floodlamp to LIGHTS_OUT
int ut02()
{
    static struct genpurpIO_register mock_reg84;   // This is masquerading as a 16 bit "register"

    // ctor initializes the floodlamp to LIGHTS_OUT
    set_bits< struct genpurpIO_register*, floodlight_t, floodlight_t > flood_light42{ &mock_reg84 }; // for lamp #42

    std::string ut_intent {  "verifing that the ctor initialized floodlamp to LIGHTS_OUT" };

    return ut_verify_lamp_state(
                                    std::string { __func__ },   // utid,
                                    ut_intent,                  // what UT is attempting to verify
                                    flood_light42(),            // flood lamp's actual settings
                                    LIGHTS_OUT                  // flood lamp's expected settings
                               );
}

// verify that the solenoid2's functor can energize solenoid2
int ut03()
{
    int something_failed = 0;

    static struct genpurpIO_register mock_reg84;   // This is masquerading as a 16 bit "register"

    set_bits< struct genpurpIO_register*, solenoid2_t, vacuum   > vac_solenoid2{ &mock_reg84 }; // for vacuum solenoid #2

    {
        std::string ut_intent {  "verifing that the solenoid2's functor can return solenoid state prior to the functor call" };

        // note1: The functor both:
        //      1) sets the solenoid to a new state; and
        //      2) returns the solenoid's state that existed prior to calling the functor.
        //
        // note2: the solenoid's expected state prior to the functor call.
        something_failed += ut_verify_solenoid_state(
                                                            std::string { __func__ },   // utid,
                                                            ut_intent,                  // what UT is attempting to verify
                                                            vac_solenoid2(vacuum::ON),  // note1
                                                            vacuum::OFF                 // note2
                                                    );
    }

    {
        std::string ut_intent {  "verifing that the solenoid2's functor can energize solenoid2" };

        something_failed += ut_verify_solenoid_state(
                                                            std::string { __func__ }, // utid,
                                                            ut_intent,                // what UT is attempting to verify
                                                            vac_solenoid2(),          // solenoid's actual state
                                                            vacuum::ON               // solenoid's expected state
                                                    );
    }

    return something_failed;
}


// verify that the solenoid3's functor can energize solenoid3
int ut04()
{
    int something_failed = 0;

    static struct genpurpIO_register mock_reg84;   // This is masquerading as a 16 bit "register"

    set_bits< struct genpurpIO_register*, solenoid3_t, vacuum   > vac_solenoid3{ &mock_reg84 }; // for vacuum solenoid #2

    {
        std::string ut_intent {  "verifing that the solenoid3's functor can return solenoid state prior to the functor call" };

        // note1: The functor both:
        //      1) sets the solenoid to a new state; and
        //      2) returns the solenoid's state that existed prior to calling the functor.
        //
        // note2: the solenoid's expected state prior to the functor call.
        something_failed += ut_verify_solenoid_state(
                                                            std::string { __func__ },   // utid,
                                                            ut_intent,                  // what UT is attempting to verify
                                                            vac_solenoid3(vacuum::ON),  // note1
                                                            vacuum::OFF                 // note2
                                                    );
    }

    {
        std::string ut_intent {  "verifing that the solenoid3's functor can energize solenoid3" };

        something_failed += ut_verify_solenoid_state(
                                                            std::string { __func__ }, // utid,
                                                            ut_intent,                // what UT is attempting to verify
                                                            vac_solenoid3(),          // solenoid's actual state
                                                            vacuum::ON                // solenoid's expected state
                                                    );
    }

    return something_failed;
}

// verify that the solenoid2's functor can de-energize solenoid2
int ut05()
{
    int something_failed = 0;

    static struct genpurpIO_register mock_reg84;   // This is masquerading as a 16 bit "register"

    set_bits< struct genpurpIO_register*, solenoid2_t, vacuum   > vac_solenoid2{ &mock_reg84 }; // for vacuum solenoid #2

    vac_solenoid2(vacuum::ON);

    {
        std::string ut_intent {  "verifing that the solenoid2's functor can return solenoid state prior to the functor call" };

        // note1: The functor both:
        //      1) sets the solenoid to a new state; and
        //      2) returns the solenoid's state that existed prior to calling the functor.
        //
        // note2: the solenoid's expected state prior to the functor call.
        something_failed += ut_verify_solenoid_state(
                                                            std::string { __func__ },   // utid,
                                                            ut_intent,                  // what UT is attempting to verify
                                                            vac_solenoid2(vacuum::OFF), // note1
                                                            vacuum::ON                  // note2
                                                    );
    }

    {
        std::string ut_intent {  "verifing that the solenoid2's functor can de-energize solenoid2" };

        something_failed += ut_verify_solenoid_state(
                                                            std::string { __func__ }, // utid,
                                                            ut_intent,                // what UT is attempting to verify
                                                            vac_solenoid2(),          // solenoid's actual state
                                                            vacuum::OFF               // solenoid's expected state
                                                    );
    }

    return something_failed;
}


// verify that the solenoid3's functor can de-energize solenoid3
int ut06()
{
    int something_failed = 0;

    static struct genpurpIO_register mock_reg84;   // This is masquerading as a 16 bit "register"

    set_bits< struct genpurpIO_register*, solenoid3_t, vacuum   > vac_solenoid3{ &mock_reg84 }; // for vacuum solenoid #2

    vac_solenoid3(vacuum::ON);

    {
        std::string ut_intent {  "verifing that the solenoid3's functor can return solenoid state prior to the functor call" };

        // note1: The functor both:
        //      1) sets the solenoid to a new state; and
        //      2) returns the solenoid's state that existed prior to calling the functor.
        //
        // note2: the solenoid's expected state prior to the functor call.
        something_failed += ut_verify_solenoid_state(
                                                            std::string { __func__ },   // utid,
                                                            ut_intent,                  // what UT is attempting to verify
                                                            vac_solenoid3(vacuum::OFF), // note1
                                                            vacuum::ON                  // note2
                                                    );
    }

    {
        std::string ut_intent {  "verifing that the solenoid3's functor can de-energize solenoid3" };

        something_failed += ut_verify_solenoid_state(
                                                            std::string { __func__ }, // utid,
                                                            ut_intent,                // what UT is attempting to verify
                                                            vac_solenoid3(),          // solenoid's actual state
                                                            vacuum::OFF                // solenoid's expected state
                                                    );
    }

    return something_failed;
}


// verify that the floodlamp's functor can set power-level
int ut07()
{
    int something_failed = 0;

    static struct genpurpIO_register mock_reg84;   // This is masquerading as a 16 bit "register"

    set_bits< struct genpurpIO_register*, floodlight_t, floodlight_t > flood_light42{ &mock_reg84 }; // for lamp #42

    {
        std::string ut_intent {  "verifing that the floodlamp's functor can return solenoid state prior to the functor call" };

        // note1: The functor both:
        //      1) sets the floodlamp's power setting to a new setting
        //      2) returns the floodlamp's power setting that existed prior to calling the functor.
        //
        // note2: the floodlamp's expected power setting prior to the functor call.
        something_failed += ut_verify_lamp_state(
                                                    std::string { __func__ },         // utid,
                                                    ut_intent,                        // what UT is attempting to verify
                                                    flood_light42(FULL_ILLUMINATION), // all bits set to 1
                                                    LIGHTS_OUT                        // note2
                                                );
    }

    {
        std::string ut_intent {  "verifing that functor can set the floodlamp to max power" };

        something_failed += ut_verify_lamp_state(
                                                    std::string { __func__ }, // utid,
                                                    ut_intent,                // what UT is attempting to verify
                                                    flood_light42(),          // obtain power setting
                                                    FULL_ILLUMINATION         // note2
                                                );
    }

    return something_failed;
}


// verify that the floodlamp's functor can set power-level
int ut08()
{
    int something_failed = 0;

    static struct genpurpIO_register mock_reg84;   // This is masquerading as a 16 bit "register"

    set_bits< struct genpurpIO_register*, floodlight_t, floodlight_t > flood_light42{ &mock_reg84 }; // for lamp #42

    {
        std::string ut_intent {  "verifing that the floodlamp's functor can return floodlamp state prior to the functor call" };

        flood_light42(FULL_ILLUMINATION), // precondition: full power

        // note1: The functor both:
        //      1) sets the floodlamp's power setting to a new setting.
        //         the next few unit tests will conduct walking 1's testing
        //      2) returns the floodlamp's power setting that existed prior to calling the functor.
        //
        // note2: the floodlamp's expected power setting prior to the functor call.
        something_failed += ut_verify_lamp_state(
                                                    std::string { __func__ },     // utid,
                                                    ut_intent,                    // what UT is attempting to verify
                                                    flood_light42(BRIGHT_LIGHTS), // walking 1's testing. note1
                                                    FULL_ILLUMINATION             // note2
                                                );
    }

    {
        std::string ut_intent {  "Walking 1's testing. power bit pattern == 0B100" };

        something_failed += ut_verify_lamp_state(
                                                    std::string { __func__ }, // utid,
                                                    ut_intent,                // what UT is attempting to verify
                                                    flood_light42(),          // obtain power setting
                                                    BRIGHT_LIGHTS             // note2
                                                );
    }

    return something_failed;
}


// verify that the floodlamp's functor can set power-level
int ut09()
{
    int something_failed = 0;

    static struct genpurpIO_register mock_reg84;   // This is masquerading as a 16 bit "register"

    set_bits< struct genpurpIO_register*, floodlight_t, floodlight_t > flood_light42{ &mock_reg84 }; // for lamp #42

    {
        std::string ut_intent {  "verifing that the floodlamp's functor can return floodlamp state prior to the functor call" };

        flood_light42(BRIGHT_LIGHTS), // precondition: power setting bit pattern == 0B100

        // note1: The functor both:
        //      1) sets the floodlamp's power setting to a new setting.
        //         the next few unit tests will conduct walking 1's testing
        //      2) returns the floodlamp's power setting that existed prior to calling the functor.
        //
        // note2: the floodlamp's expected power setting prior to the functor call.
        something_failed += ut_verify_lamp_state(
                                                    std::string { __func__ },     // utid,
                                                    ut_intent,                    // what UT is attempting to verify
                                                    flood_light42(MOOD_LIGHTING), // walking 1's testing. note1
                                                    BRIGHT_LIGHTS                 // note2
                                                );
    }

    {
        std::string ut_intent {  "Walking 1's testing. power bit pattern == 0B010" };

        something_failed += ut_verify_lamp_state(
                                                    std::string { __func__ }, // utid,
                                                    ut_intent,                // what UT is attempting to verify
                                                    flood_light42(),          // obtain power setting
                                                    MOOD_LIGHTING             // note2
                                                );
    }

    return something_failed;
}


// verify that the floodlamp's functor can set power-level
int ut10()
{
    int something_failed = 0;

    static struct genpurpIO_register mock_reg84;   // This is masquerading as a 16 bit "register"

    set_bits< struct genpurpIO_register*, floodlight_t, floodlight_t > flood_light42{ &mock_reg84 }; // for lamp #42

    {
        std::string ut_intent {  "verifing that the floodlamp's functor can return floodlamp state prior to the functor call" };

        flood_light42(MOOD_LIGHTING), // precondition: power setting bit pattern == 0B010

        // note1: The functor both:
        //      1) sets the floodlamp's power setting to a new setting.
        //         the next few unit tests will conduct walking 1's testing
        //      2) returns the floodlamp's power setting that existed prior to calling the functor.
        //
        // note2: the floodlamp's expected power setting prior to the functor call.
        something_failed += ut_verify_lamp_state(
                                                    std::string { __func__ },       // utid,
                                                    ut_intent,                      // what UT is attempting to verify
                                                    flood_light42(VERY_DIM_LIGHTS), // walking 1's testing. note1
                                                    MOOD_LIGHTING                   // note2
                                                );
    }

    {
        std::string ut_intent {  "Walking 1's testing. power bit pattern == 0B001" };

        something_failed += ut_verify_lamp_state(
                                                    std::string { __func__ }, // utid,
                                                    ut_intent,                // what UT is attempting to verify
                                                    flood_light42(),          // obtain power setting
                                                    VERY_DIM_LIGHTS           // note2
                                                );
    }

    return something_failed;
}


// verify that the floodlamp's functor can set power-level
int ut11()
{
    int something_failed = 0;

    static struct genpurpIO_register mock_reg84;   // This is masquerading as a 16 bit "register"

    set_bits< struct genpurpIO_register*, floodlight_t, floodlight_t > flood_light42{ &mock_reg84 }; // for lamp #42

    {
        std::string ut_intent {  "verifing that the floodlamp's functor can return floodlamp state prior to the functor call" };

        flood_light42(VERY_DIM_LIGHTS), // precondition: power setting bit pattern == 0B010

        // note1: The functor both:
        //      1) sets the floodlamp's power setting to a new setting.
        //         the next few unit tests will conduct walking 1's testing
        //      2) returns the floodlamp's power setting that existed prior to calling the functor.
        //
        // note2: the floodlamp's expected power setting prior to the functor call.
        something_failed += ut_verify_lamp_state(
                                                    std::string { __func__ },  // utid,
                                                    ut_intent,                 // what UT is attempting to verify
                                                    flood_light42(LIGHTS_OUT), // walking 1's testing. note1
                                                    VERY_DIM_LIGHTS            // note2
                                                );
    }

    {
        std::string ut_intent {  "Verify that functor can remove power from floodlamp" };

        something_failed += ut_verify_lamp_state(
                                                    std::string { __func__ }, // utid,
                                                    ut_intent,                // what UT is attempting to verify
                                                    flood_light42(),          // obtain power setting
                                                    LIGHTS_OUT                // note2
                                                );
    }

    return something_failed;
}

// Floodlamp Out of range exception
int ut12()
{
    int something_failed = 1;    // init to UT failure

    static struct genpurpIO_register mock_reg84;   // This is masquerading as a 16 bit "register"

    set_bits< struct genpurpIO_register*, floodlight_t, floodlight_t > flood_light42{ &mock_reg84 }; // for lamp #42

    std::cout << std::string { __func__ } << ": ";  // output utid to stdout

    std::stringstream ut_intent {};
    ut_intent <<  "Verifing floodlamp's 'Out of Range' exception.";
    ut_intent <<  "......................................................";   // note4

    try
    {
        // attempt to set floodlamp power level beyond possible max power settings.
        flood_light42(FLOODLIGHT_OOR); // should throw out of range exception here

        // still here? Then the functor failed to throw an Out Of Range exception
        std::string tmp { ut_intent.str()  };
        tmp.insert( ok_col_pos, "FAILED!" );    // columnize "FAILED!" text   See note1
        rtrim(tmp);                             // toss trailing dots
        std::cout << tmp << std::endl;
    }
    catch (std::range_error & e)
    {
        // the floodlamp functor successfully threw a std::range_error when
        // it was passed an illegal power level
        //
        // if we are here then the UT passed
        something_failed = 0;    // indicate UT passed

        std::string tmp { ut_intent.str()  };
        tmp.insert( ok_col_pos, "ok" );     // columnize "ok" text   See note1
        rtrim(tmp);                         // toss trailing dots
        std::cout << tmp << std::endl;
    }

    return something_failed;
}



//-----------------------------------------------------

int main( int argc, char * argv[] )
{
    static struct genpurpIO_register mock_reg84;   // This is masquerading as a 16 bit "register"

    set_bits< struct genpurpIO_register*, solenoid2_t, vacuum   > vac_solenoid2{ &mock_reg84 }; // for vacuum solenoid #2
    set_bits< struct genpurpIO_register*, solenoid3_t, vacuum   > vac_solenoid3{ &mock_reg84 }; // for vacuum solenoid #3
    set_bits< struct genpurpIO_register*, floodlight_t, floodlight_t > flood_light42{ &mock_reg84 }; // for lamp #42

    bool something_failed = false;
    try
    {
        // something_failed += deliberately_throw_exception();  // note2
        //
        //-------------------------------------------------------------
        //
        // UT ctor
        //
        something_failed += ut00();     // verify that the ctor sets solenoid2 to vacuum:OFF
        something_failed += ut01();     // verify that the ctor sets solenoid3 to vacuum:OFF
        something_failed += ut02();     // verify that the ctor sets floodlamp to LIGHTS_OUT
        //
        //-------------------------------------------------------------
        //
        // UT solenoid functors
        //
        something_failed += ut03();     // verify solenoid2's functor
        something_failed += ut04();     // verify solenoid3's functor
        something_failed += ut05();     // verify solenoid2's functor
        something_failed += ut06();     // verify solenoid3's functor
        //
        //-------------------------------------------------------------
        //
        // Floodlamp functors
        //
        something_failed += ut07();     // verify floodlamp's functor can set floodlamp to max power
        something_failed += ut08();     // walking 1's testing: floodlamp's power-level == 100
        something_failed += ut09();     // walking 1's testing: floodlamp's power-level == 010
        something_failed += ut10();     // walking 1's testing: floodlamp's power-level == 001
        something_failed += ut11();     // verify floodlamp's functor remove power from floodlamp
        //
        //-------------------------------------------------------------
        //
        // Floodlamp Out of range exception
        //
        something_failed += ut12();     // Floodlamp Out of range exception

    }
    catch (std::exception& e)
    {
        std::cerr << std::endl << "UNEXPECTED exception thrown: " << e.what() << std::endl << std::endl;
        something_failed = 1;
    }

    if ( something_failed )
    {
        // note3:   One of the desired affects of a UT failure to
        //          "break the build". In this way UT failures
        //          cannot be ignored.
        //
        //          make aborts processing a Makefile wherever one of the
        //          Makefile's recipes terminates with non-zero exit status.
        //
        //          Consequently, this app must terminate with a
        //          non-zero exit status when any unit if its tests fail
        //          so as to break the build on UT failure.
        //
        std::cerr << std::endl << "UNIT TEST FAILED!" << std::endl;
        exit(1);   // note3
    }
    else
    {
        std::cout << std::endl << "UNIT TEST passed!" << std::endl;
    }
    return 0;

#if 0
    try
    {
        vac_solenoid2(vacuum::ON);
        vac_solenoid3(vacuum::ON);
        flood_light42(FULL_ILLUMINATION);

        assert (vac_solenoid2() == vacuum::ON);
        assert (vac_solenoid3() == vacuum::ON);
        assert (flood_light42() == FULL_ILLUMINATION);

        // the next three flood light values conduct a
        // 'walking ones' test on the flood light field.
        //
        // In real life, during manufacturing testing,
        // this test pattern is useful for verifying
        // that all three bits on the flood lamp's data
        // bus are functional, no bits are stuck high,
        // none are shorted together, and so on.
        //

        vac_solenoid2(vacuum::OFF);
        vac_solenoid3(vacuum::OFF);
        flood_light42(BRIGHT_LIGHTS);         // (walking 1s test)

        assert (vac_solenoid2() == vacuum::OFF);
        assert (vac_solenoid3() == vacuum::OFF);
        assert (flood_light42() == BRIGHT_LIGHTS);

        vac_solenoid2(vacuum::ON);
        vac_solenoid3(vacuum::OFF);
        flood_light42(MOOD_LIGHTING);         //  (walking 1s test)

        assert (vac_solenoid2() == vacuum::ON);
        assert (vac_solenoid3() == vacuum::OFF);
        assert (flood_light42() == MOOD_LIGHTING);

        vac_solenoid2(vacuum::ON);
        vac_solenoid3(vacuum::ON);
        flood_light42(VERY_DIM_LIGHTS);         // (walking 1s test)

        assert (vac_solenoid2() == vacuum::ON);
        assert (vac_solenoid3() == vacuum::ON);
        assert (flood_light42() == VERY_DIM_LIGHTS);

        vac_solenoid2(vacuum::OFF);
        vac_solenoid3(vacuum::ON);
        flood_light42(LIGHTS_OUT);

        assert (vac_solenoid2() == vacuum::OFF);
        assert (vac_solenoid3() == vacuum::ON);
        assert (flood_light42() == LIGHTS_OUT);
    }
    catch (std::exception & e)
    {
        std::cerr << "Exception thrown: ";
        std::cerr << e.what() << std::endl;
        exit(EXIT_FAILURE);
    }

    // still here? then the UT passed.

    std::cerr << std::endl;
    std::cerr << "unit test passed " << std::endl;
    std::cerr << std::endl;

    return 0;
#endif
}

