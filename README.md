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

1. functors for a GPIO register that is part of a custom design (e.g., a custom controller board); and
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

The address of the GPIO register is passed via the functor's constructor (RAII). Afterward the functor's sole parameter is value used to update the field managed by the given functor.  

## About portabilty:
The layout of the field within a bitfield is implementation dependent. Consequently, this code cannot be expected to be portable between C++ compilers.

This is probably OK for the purposes of embedded programming b/c in the varous shops in which I worked portablity wasn't a concern because each shop used only used one particular C++ compiler for the particular microcontroller they were using.


## Annoyances:

1. Each field within the bit-field requires its own partial specialization of the class template, even if there are two fields with exactly the same usage (scenario: each of the two vacuum solenoids had a distinct field.) This annoyed because it meant code duplication, which defeats the recommendation to keep your code DRY.

2. I needed to introduce extra types just to create distinct partial specialization for each named field. The field names themselves were inadequate for this purpose because they were the same type (std::uint16_t) and each partial specialization requires a distinct type.  This annoyed because I had to declare additional, otherwise pointless type just because of the way partial specialization works.

## Tradeoffs: creating a partial specialization of a class template for each field vs creating a class with a method for each field.

1. Implementing this via, say, a class would not help the code duplication concern because the implementation of the methods for the vacuum solenoids would still mean two distinct member functions, one for each vacuum solenoid. (wash)
2. Implementing this via, say, a class would remove the need to create types just for the purpose of creating a distinct partial specialization for each solenoid.  (class wins)
3. Each functors must be individually instantiated while a class need only be instantiated once. (class wins)
4. The call to the functor() appears to be slightly less complex than a call to a class member function. For example
````
   obj.vac_solenoid2(vacuum::ON);
vs
   vac_solenoid2(vacuum::ON);

````
Consequently, a functor call appears to be indistinguishable from calling a C function. (And does anything appears simpler than a C function call?)
(templates win)

Conclusion:

I cannot, with a straight face, make an argument that either approach has a particularly strong advantage.

That said, one important design principle is that it is permissible to make the implementation somewhat more complex in exchange for simplifying the iterface. This consideration becomes more compelling if the usage of a functor occurs many more times that the effort required to setup a functor.

Consequently, I am willing to rationalize that functors are a more preferred implementation because calling functors appears as simple as calling a C function.

## Regarding this example's NEED_IC_ID 

  In the case of an given mass-manufactured device, one that is not
  field programmable so that its registers usage is customizable,
  as in the case of NEED_IC_ID, the said device's register layouts will always
  be the same.  In this sort of case code reuse is practical.


[Need to add commentary regarding NEED_IC_ID]


# Getting Started

These instructions will get you a copy of the project up and running on your local machine for development and testing purposes. See deployment for notes on how to deploy the project on a live system.
Prerequisites

# What things you need to install the software and how to install them

Give examples

# Installing

A step by step series of examples that tell you how to get a development env running

Say what the step will be

Give the example

And repeat

until finished

End with an example of getting some data out of the system or using it for a little demo
# Running the tests

Explain how to run the automated tests for this system
Break down into end to end tests

Explain what these tests test and why

Give an example

And coding style tests

Explain what these tests test and why

Give an example

# Deployment

Add additional notes about how to deploy this on a live system
Built With

    Dropwizard - The web framework used
    Maven - Dependency Management
    ROME - Used to generate RSS Feeds

# Contributing

Please read CONTRIBUTING.md for details on our code of conduct, and the process for submitting pull requests to us.
Versioning

We use SemVer for versioning. For the versions available, see the tags on this repository.
Authors

    Billie Thompson - Initial work - PurpleBooth

See also the list of contributors who participated in this project.
License

This project is licensed under the MIT License - see the LICENSE.md file for details
# Acknowledgments

    Hat tip to anyone whose code was used
    Inspiration
    etc
