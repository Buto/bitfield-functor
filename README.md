# bitfield-functor
# Summary

Embedded programing proof of concept: bitfields with functors

 My motivations for conducting this exercise was to experiment with using functors for twiddling bits in a GPIO register. I haven't noticed anyone doing this and I was just interested in seeing how this approach would work out.
# Extended Commentary
Normally hiding bit-twiddling would just mean abstracting such code into one or more functions.  This time I wanted to abstract such code into functors instead of functions.  At the call site both functions and functors would appear indistinguishable from each other.

In addition, this time I wanted to use bit-fields instead of the "raw bits" approach (i.e., bitwise ANDing,ORing and so on.)  NB: I've always understood bit-fields perfectly well, it's just that everyone else almost always chose to go with the "raw bits" technique.  That said, I've always considered  bit-fields to be a more natural looking way to twiddle bits in a register.

 So, in short, I had the following objectives:

1. I wanted to work with functors
2. I wanted to work with partial template specialization
3. I wanted to try using bit-fields instead of the "raw bits" technique
4. I wanted to discover how much extra code is required to go the generic programming template route.

## The two applications for this template/bitfield scheme
 There are two scenarios I wanted to cover in this exercise:

1. functors for a GPIO register that is part of a custom design (e.g., a custom controller board);  and
2. functors for the registers of an off-the-shelf IC (e.g., NEED_IC_ID)
## Regarding this example's GPIO register
This GPIO register controls:
1. two solenoids, each of which operate a valve on a vaccum line; and
2. a lamp's brightness.

The audence should assume the necessary external circuitry is implemented. For example:
1. Assume that the bits associated with controlling the vaccum solenoids are connected to circuity (relay drivers, most likely) suitable for controlling power to a solenoid's coil.
2. Assume that the bits associated with controlling the lights are connected to a DAC, followed by suitable power control circuitry for modulating the power applied to the lamp. 
3. Assume that the lamp's illumination is exactly proportional to power applied (which cannot be expected with incandescent lamps.)
## Regarding the template for the GPIO register

The irony of using a template to operate on a GPIO register hardwired to a custom design can be summed up in two points:

1. The point of templates is to facilitate generic programming;
2. I cannot conceive of a way to abstract bitfield usage in such a way that one template can accomodate all designs for using a GPIO register.

Put another way, the highly hardware implementation specific concerns of register bit-twiddling defeats generic programming's intent of code reuse.  Consequently, the template code for something like a GPIO register will be a one-off, unreusable code.

Unresusable code means that you're not much better off than writing bit manipulation functions in C as compared to templates, at least with respect to the number of lines of code.

This means you cannot expect to reuse functors created for customizeable registers on other, unreleated, designs. Of course this code might be reusable should the design team reuse the hardware design pertaining to the GPIO register, say, on another version of the control board.

## Specializing the templates for the GPIO register's named bitfields 
This example's GPIO register's bitfield is defined:

```
struct genpurpIO_register23
{
     std::uint16_t energize_vac_solenoid2 : 1;    // set to 1 to apply vacuum
     std::uint16_t energize_vac_solenoid3 : 1;    // set to 1 to apply vacuum
     std::uint16_t lamp_pwr               : 3;    // 0 == lights off; 7 == max illumination
     std::uint16_t                        : 11;   // fill to 16 bits
};
```
I will refer to the fields with identifiers (meaning all fields in the bitfield execpt for the _filler_ field) as _named fields_. 

Each functor is implemented via a partial template specialization. Each functor manages exactly one named field. Each functor is defined in its own _partial template specialization_.  Consequently, each named field gets its own partial template specialization.

These partial template specialization only have a single parameter: a unique type only used to select it from the set of available partial template specializations. 

## Annoyances:

1. Each field within the bit-field requires its own partial specialization of the class template, even if there are two fields with exactly the same usage (scenario: each of the two vacuum solenoids had a distinct field.) This annoyed because it meant code duplication, which defeats the recommendation to keep your code DRY.

2. I needed to introduce extra types just to create distinct partial specialization for each named field. The field names themselves were inadequate for this purpose because they were the same type (std::uint16_t) and each partial specialization requires a distinct type.  This annoyed because I had to declare additional, otherwise pointless type just because of the way partial specialization works.



```
//-------- These typedefs only exist to instantiate partial specializations ----------

typedef struct solenoid2 * solenoid2_t;
typedef struct solenoid3 * solenoid3_t;
typedef std::uint16_t      lamp_t;

template<>
class gpio_register_23< solenoid2_t  >
{ ... }

```



