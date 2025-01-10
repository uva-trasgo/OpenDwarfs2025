/****************************************************************************
 * GEM -- electrostatics calculations and visualization                     *
 * Copyright (C) 2006  John C. Gordon                                       *
 *                                                                          *
 * This program is free software; you can redistribute it and/or modify     *
 * it under the terms of the GNU General Public License as published by     *
 * the Free Software Foundation; either version 2 of the License, or        *
 * (at your option) any later version.                                      *
 *                                                                          *
 * This program is distributed in the hope that it will be useful,          *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 * GNU General Public License for more details.                             *
 *                                                                          *
 * You should have received a copy of the GNU General Public License along  *
 * with this program; if not, write to the Free Software Foundation, Inc.,  *
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.              *
 ****************************************************************************/
/*****************************************************************************
 * This file contains the main functionality of the program, it also contains*
 * the main() function. --jcg                                                *
 ****************************************************************************/
#include <string>
#include <stdio.h>
#include <iostream>
#include <string.h>
#include <ctype.h>
#include <vector>
#include "defines.h"
#include "gem.h"

using namespace std;

/* checks to see if a string is a number */
int stringIsNum(string num)
{
   /* local variables */
   int i(0), length;
   bool found_decimal(false);

   length = (int) num.length();

   if (length == 0) return 0;

   /* optional leading sign there */
   if ((num[i] == '+')||(num[i] == '-'))
      i++;

   /* this had better be a number... */
   for (; i < length; i++)
   {
      if (isdigit(num[i]))
         continue;
      else if ((num[i] == '.')&&(!found_decimal))
         found_decimal = true;
      else
         return 0;
   }

   return ((found_decimal)?1:2);
}

/* this function checks the command line input for errors and extracts pertinent
 * information where possible returns false if necessary info is missing --jcg*/
bool check_cmd_line(int argc, char **argv, string *files, bool *flags)
{
   /* local variables */
   bool found;
   int i, j; /* loop counters */
   static string flagmarkers[] = {"-readPhi", "-writePhi", "-visualize",
                                  "-projLen", "-triDens",
                                  "-fixA", "-dumpImage", "-comparePhi",
                                  "-dumpVertices", "-probeRad", "-fixColors",
                                  "-decorMol", "-decorType", "-bg","-p","-d"};
   string currArg;

   /* initialize flags */
   for (i = 0; i < NUM_FLAGS; i++)
      flags[i] = false;

   /* check args 2 - 4, make sure they are numbers */
   for (i = 2; i < 5; i++)
   {
      currArg = argv[i];
      if (!stringIsNum(currArg))
         return false;

   }/* end for loop */

   /* look for optional arguments */
   for (i = 5; i < argc; i++)
   {
      currArg = argv[i];
      found = false;

      for (j = 0; j < NUM_FLAGS; j++)
      {
         if (currArg == flagmarkers[j])
         {
            flags[j] = true;
            i++;

            switch(j)
            {
               case READ_PHI:
                 if (i >= argc)
                    return false;
                 else
                    files[PHI_IN_FILE] = argv[i];
               break;

               case WRITE_PHI:
                 if (i+2 >= argc)
                    return false;
                 else
                 {
                    files[PHI_OUT_FILE] = argv[i];
                    files[PHI_WID_VAL]  = argv[i+1];
                    files[PHI_EDGE_VAL] = argv[i+2];
                    i += 2;
                 }
               break;
	       case P_ID:
		    files[P_ID_VAL] = argv[i];
		    break;
	       case D_ID:
		    files[D_ID_VAL] = argv[i];
	            break;
               case PROJ_SPEC:
                  if (i >= argc)
                     return false;
                  else
                     files[PROJ_LENGTH_VAL] = argv[i];
               break;

               case TRI_SPEC:
                  if (i >= argc)
                     return false;
                  else
                     files[TRI_SPEC_VAL] = argv[i];
               break;

               case PROBE_SPEC:
                  if (i >= argc)
                     return false;
                  else
                     files[PROBE_RADIUS_VAL] = argv[i];
               break;

               case A_SPEC:
                  if (i >= argc)
                     return false;
                  else
                  {
                     if (stringIsNum(string(argv[i])))
                     {
                        files[A_SPEC_VAL] = argv[i];
                     }
                     else
                     {
                        cerr<<"ERROR: -fixA requires a number"<<endl;
                        return false;
                     }
                  }
                        
               break;

               case DUMP_IMAGE:
                  if (i+2 >= argc)
                  {
                     cerr<<"ERROR: Not enough arguments for -dumpImage"<<endl;
                     return false;
                  }
                  else
                  {
                     if ((stringIsNum(string(argv[i+1])) == 2)
                         && (stringIsNum(string(argv[i+2])) == 2))
                     {
                        files[IMAGE_VALS] = argv[i];
                        files[IMAGE_VALS] +="\n";
                        files[IMAGE_VALS] += argv[i+1];
                        files[IMAGE_VALS] +="\n";
                        files[IMAGE_VALS] += argv[i+2];
                        i+=2;
                     }
                     else
                     {
                        cerr<<"ERROR: Non-integer dimension arguments for -dumpImage"<<endl;
                        return false;
                     }

                  }
               break;

               case COMPARE_PHI:
                  if (i >= argc)
                     return false;
                  else
                     files[PHI_COMPARE_FILE] = argv[i];
               break;

               case DUMP_VERTICES:
                  if (i >= argc)
                     return false;
                  else
                     files[VERTEX_FILE_NAME] = argv[i];
               break;

               case FIX_COLOR_SCALE:
                  if (i >= argc)
                     return false;
                  else
                  {
                     if (stringIsNum(string(argv[i])))
                     {
                        files[COLOR_SCALE_VAL] = argv[i];
                     }
                     else
                     {
                        cerr<<"ERROR: -fixColors requires a number"<<endl;
                        return false;
                     }
                  }
               break;

               case DECOR_MOLECULE:
                  if (i >= argc)
                     return false;
                  else
                     files[DECOR_MOL_NAME] = argv[i];
               break;

               case DECOR_TYPE:
                  if (i >= argc)
                     return false;
                  else
                     files[DECOR_MOL_LOOK] = argv[i];
               break;

               case BG_COLOR:
                  if (i+2 >= argc)
                  {
                     cerr<<"ERROR: Not enough arguments for -bg"<<endl;
                     return false;
                  }
                  else
                  {
                     if (stringIsNum(string(argv[i+1]))
                         && stringIsNum(string(argv[i+2])))
                     {
                        files[BG_COLOR_RGB] = argv[i];
                        files[BG_COLOR_RGB] +="\n";
                        files[BG_COLOR_RGB] += argv[i+1];
                        files[BG_COLOR_RGB] +="\n";
                        files[BG_COLOR_RGB] += argv[i+2];
                        i+=2;
                     }
                     else
                     {
                        cerr<<"ERROR: Non-number color arguments for -bg (needs 3 floating point values between 0 and 1)"<<endl;
                        return false;
                     }

                  }
               break;

               default:
                     i--;

            }/* end switch on i */

            found = true;

         }/* end if */

      }/* end for (j < NUM_FLAGS) */
      if (!found)
      {
         printf("Unknown option: %s\n", argv[i]);
      }

   }/* end for (i < argc) */

   /* everything must be ok then... */
   return true;

}/* end function check_cmd_line */



/** This function verifies that the las 3 args are numbers.
 *  The code is the same one from chec_cmd_line() used for same purpose**/
bool verify_initial_args(int argc, char **argv){
      /* check args 2 - 4, make sure they are numbers */
   for (int i = 2; i < 5; i++)
   {
      if (!stringIsNum(argv[i]))
         return false;

   }/* end for loop */
      return true;
}


/* this function is a remodel of the original function used to check arguments
   While it might not be the best, it is more easy to work with and maintain
   Only contains options that where in the README of GEM on OpenDwarfs*/
void read_opt_arg_cmd_line(int argc, char **argv, string *files,bool *flags, size_t* localSZ)
{
   
   int opt = 0;
   int i;
      /* initialize flags */
   for ( i = 0; i < NUM_FLAGS; i++)
      flags[i] = false;

   /** Manage options if theres any**/
   for ( i = 5;(i<argc); i += 2)
   {
      opt = check_option(argv[i]);
      switch(opt){

         case -1:
            printf("Theres some error");
            exit(EXIT_FAILURE);
         break;

         case 0://-projLen <>

            files[PROJ_LENGTH_VAL] = argv[i+1];
            flags[PROJ_SPEC] = true;

         break;
         
         case 1://-triDens <>

            files[TRI_SPEC_VAL] = argv[i+1];
            flags[TRI_SPEC] = true;

         break;
         
         case 2://-probeRad <>

            files[PROBE_RADIUS_VAL] = argv[i+1];
            flags[PROJ_SPEC] = true;

         break;
         
         case 3://-fixA <number>

            if (stringIsNum(string(argv[i+1])))
            {
               files[A_SPEC_VAL] = atoi(argv[i+1]);
               flags[A_SPEC] = true;
            }
            else
            {
               cerr<<"ERROR: -fixA requires a number"<<endl;
               exit(EXIT_FAILURE);
            }
         
         break;
         
         case 4://-dumpVertices
            
            if((i+1)<argc){
               files[VERTEX_FILE_NAME] = argv[i+1];
               flags[DUMP_VERTICES] = true;
            }else //if user inputs the option but no name for the file
            {
               files[VERTEX_FILE_NAME] = "phi.out";
               flags[DUMP_VERTICES] = true;
            }

         break;
         
         case 5://-localSize
            *localSZ = (size_t) atoi(argv[i+1]);//Global var in gem_no_ui.cpp
            printf("WIP. Should allow to choose the localSize de dwarf GEMOUI. Pending to Verify\n");
         break;

         default:
            printf("Error. Option not Found.Exiting dwarfs.\n");
            exit(EXIT_FAILURE);
         break;
      }
   }  
} 


/** This functions is a simple loop to convert and more easily manage cmd_lines options.*/
int check_option(char *argv){
   vector<const char*> options  = {"-projLen", "-triDens", 
                       "-probeRad","-fixA","-dumpVertices","-localSize"};
   int listOptions = 6;

   for(int ii=0;ii<listOptions;ii++)
      if(strcmp(argv,options[ii]) == 0)
         return ii;

   return -1;
}