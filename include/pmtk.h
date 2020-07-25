/*
* PMTK commands for use with MT3399 GNSS platform SOC
*/

#include "mgos.h"



#include <stdbool.h>

enum pmtk_sentence_id {
  PMTK_UNKNOWN = -2,
  PMTK_INVALID = -1,
  PMTK_TEST,
  PMTK_ACK,
  PMTK_SYS_MSG
};

enum pmtk_ack_id {
  PMTK_ACK_INVALID_COMMAND,
  PMTK_ACK_UNSUPPORTED_COMMAND,
  PMTK_ACK_ACTION_FAILED,
  PMTK_ACK_ACTION_SUCCEEDED,
  
};


#define PMTK_TEST 000
#define PMTK_ACK 001
#define PMTK_SYS_MSG 010


#define PMTK_ACK_INVALID_COMMAND 0x0000
#define PMTK_ACK_UNSUPPORTED_COMMAND 0x0001
#define PMTK_ACK_ACTION_FAILED 0x0002
#define PMTK_ACK_ACTION_SUCCEEDED 0x0003


struct pmtk_sentence_ack {
  int command_ackd;
  enum pmtk_ack_id flag;
};

enum pmtk_sentence_id pmtk_sentence_id(const char *sentence, bool strict);

bool pmtk_parse_ack(struct pmtk_sentence_ack *frame, const char *sentence);

//bool parsePmtkString(const char* line, struct gps2 *gps_dev);

/**
 Different commands to set the update rate from once a second (1 Hz) to 10 times
 a second (10Hz) Note that these only control the rate at which the position is
 echoed, to actually speed up the position fix you must also send one of the
 position fix rate commands below too. */
#define PMTK_SET_NMEA_UPDATE_100_MILLIHERTZ                                    \
  "$PMTK220,10000*2F" ///< Once every 10 seconds, 100 millihertz.
#define PMTK_SET_NMEA_UPDATE_200_MILLIHERTZ                                    \
  "$PMTK220,5000*1B" ///< Once every 5 seconds, 200 millihertz.
#define PMTK_SET_NMEA_UPDATE_1HZ "$PMTK220,1000*1F" ///<  1 Hz
#define PMTK_SET_NMEA_UPDATE_2HZ "$PMTK220,500*2B"  ///<  2 Hz
#define PMTK_SET_NMEA_UPDATE_5HZ "$PMTK220,200*2C"  ///<  5 Hz
#define PMTK_SET_NMEA_UPDATE_10HZ "$PMTK220,100*2F" ///< 10 Hz
// Position fix update rate commands.
#define PMTK_API_SET_FIX_CTL_100_MILLIHERTZ                                    \
  "$PMTK300,10000,0,0,0,0*2C" ///< Once every 10 seconds, 100 millihertz.
#define PMTK_API_SET_FIX_CTL_200_MILLIHERTZ                                    \
  "$PMTK300,5000,0,0,0,0*18" ///< Once every 5 seconds, 200 millihertz.
#define PMTK_API_SET_FIX_CTL_1HZ "$PMTK300,1000,0,0,0,0*1C" ///< 1 Hz
#define PMTK_API_SET_FIX_CTL_5HZ "$PMTK300,200,0,0,0,0*2F"  ///< 5 Hz
// Can't fix position faster than 5 times a second!

#define PMTK_SET_BAUD_115200 "$PMTK251,115200*1F" ///< 115200 bps
#define PMTK_SET_BAUD_57600 "$PMTK251,57600*2C"   ///<  57600 bps
#define PMTK_SET_BAUD_9600 "$PMTK251,9600*17"     ///<   9600 bps

#define PMTK_SET_NMEA_OUTPUT_GLLONLY                                           \
  "$PMTK314,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*29" ///< turn on only the
                                                      ///< GPGLL sentence
#define PMTK_SET_NMEA_OUTPUT_RMCONLY                                           \
  "$PMTK314,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*29" ///< turn on only the
                                                      ///< GPRMC sentence
#define PMTK_SET_NMEA_OUTPUT_VTGONLY                                           \
  "$PMTK314,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*29" ///< turn on only the
                                                      ///< GPVTG
#define PMTK_SET_NMEA_OUTPUT_GGAONLY                                           \
  "$PMTK314,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*29" ///< turn on just the
                                                      ///< GPGGA
#define PMTK_SET_NMEA_OUTPUT_GSAONLY                                           \
  "$PMTK314,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0*29" ///< turn on just the
                                                      ///< GPGSA
#define PMTK_SET_NMEA_OUTPUT_GSVONLY                                           \
  "$PMTK314,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0*29" ///< turn on just the
                                                      ///< GPGSV
#define PMTK_SET_NMEA_OUTPUT_RMCGGA                                            \
  "$PMTK314,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*28" ///< turn on GPRMC and
                                                      ///< GPGGA
#define PMTK_SET_NMEA_OUTPUT_RMCGGAGSA                                         \
  "$PMTK314,0,1,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0*29" ///< turn on GPRMC, GPGGA
                                                      ///< and GPGSA
#define PMTK_SET_NMEA_OUTPUT_ALLDATA                                           \
  "$PMTK314,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0*28" ///< turn on ALL THE DATA
#define PMTK_SET_NMEA_OUTPUT_OFF                                               \
  "$PMTK314,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*28" ///< turn off output

// to generate your own sentences, check out the MTK command datasheet and use a
// checksum calculator such as the awesome
// http://www.hhhh.org/wiml/proj/nmeaxor.html

#define PMTK_LOCUS_STARTLOG "$PMTK185,0*22" ///< Start logging data
#define PMTK_LOCUS_STOPLOG "$PMTK185,1*23"  ///< Stop logging data
#define PMTK_LOCUS_STARTSTOPACK                                                \
  "$PMTK001,185,3*3C" ///< Acknowledge the start or stop command
#define PMTK_LOCUS_QUERY_STATUS "$PMTK183*38"  ///< Query the logging status
#define PMTK_LOCUS_ERASE_FLASH "$PMTK184,1*22" ///< Erase the log flash data
#define LOCUS_OVERLAP                                                          \
  0 ///< If flash is full, log will overwrite old data with new logs
#define LOCUS_FULLSTOP 1 ///< If flash is full, logging will stop

#define PMTK_ENABLE_SBAS                                                       \
  "$PMTK313,1*2E" ///< Enable search for SBAS satellite (only works with 1Hz
                  ///< output rate)
#define PMTK_ENABLE_WAAS "$PMTK301,2*2E" ///< Use WAAS for DGPS correction data

#define PMTK_STANDBY                                                           \
  "$PMTK161,0*28" ///< standby command & boot successful message
#define PMTK_STANDBY_SUCCESS "$PMTK001,161,3*36" ///< Not needed currently
#define PMTK_AWAKE "$PMTK010,002*2D"             ///< Wake up

#define PMTK_Q_RELEASE "$PMTK605*31" ///< ask for the release and version

#define PGCMD_ANTENNA                                                          \
  "$PGCMD,33,1*6C" ///< request for updates on antenna status
#define PGCMD_NOANTENNA "$PGCMD,33,0*6D" ///< don't show antenna status messages



/* send a PMTK command_string to the global GPS check and wait ack_timeout milliseconds 
 * returns */ 
int gps2_send_pmtk_command(struct mg_str command_string);

