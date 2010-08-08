/* vi: set ts=2:
 +-------------------+  
 |                   |  Enno Rehling <enno@eressea.de>
 | Eressea PBEM host |  Katja Zedel <katze@felidae.kn-bremen.de>
 | (c) 1998 - 2010   |  Christian Schlittchen <corwin@amber.kn-bremen.de>
 |                   |  
 +-------------------+  

 This program may not be used, modified or distributed 
 without prior permission by the authors of Eressea.
*/

#ifndef H_UTIL_EVTBUS
#define H_UTIL_EVTBUS


#ifdef __cplusplus
extern "C" {
#endif

  typedef void (*event_handler)(void *, const char *, void *);
  typedef void (*event_arg_free)(void *);
  void eventbus_fire(void * sender, const char * event, void * args);
  void eventbus_register(void * sender, const char * event, event_handler callback, event_arg_free arg_free, void * args);

#ifdef __cplusplus
}
#endif
#endif
