//
// Created by Anders Cedronius on 2020-01-31.
//

#ifndef EFPBOND__BONDING_H
#define EFPBOND__BONDING_H

// GLobal Logger -- Start
#define LOGG_NOTIFY (unsigned)1
#define LOGG_WARN (unsigned)2
#define LOGG_ERROR (unsigned)4
#define LOGG_FATAL (unsigned)8
#define LOGG_MASK LOGG_NOTIFY | LOGG_WARN | LOGG_ERROR | LOGG_FATAL //What to logg?

#ifdef DEBUG
#define LOGGER(l, g, f) \
{ \
std::ostringstream a; \
if (g == (LOGG_NOTIFY & (LOGG_MASK))) {a << "Notification: ";} \
else if (g == (LOGG_WARN & (LOGG_MASK))) {a << "Warning: ";} \
else if (g == (LOGG_ERROR & (LOGG_MASK))) {a << "Error: ";} \
else if (g == (LOGG_FATAL & (LOGG_MASK))) {a << "Fatal: ";} \
if (a.str().length()) { \
if (l) {a << __FILE__ << " " << __LINE__ << " ";} \
a << f << std::endl; \
std::cout << a.str(); \
} \
}
#else
#define LOGGER(l,g,f)
#endif
// GLobal Logger -- End


#endif //EFPBOND__BONDING_H
