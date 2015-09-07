#!/usr/bin/perl
# $Id: lmsc.pl,v 1.26 Broadcom SDK $
#
# $Copyright: Copyright 2012 Broadcom Corporation.
# This program is the proprietary software of Broadcom Corporation
# and/or its licensors, and may only be used, duplicated, modified
# or distributed pursuant to the terms and conditions of a separate,
# written license agreement executed between you and Broadcom
# (an "Authorized License").  Except as set forth in an Authorized
# License, Broadcom grants no license (express or implied), right
# to use, or waiver of any kind with respect to the Software, and
# Broadcom expressly reserves all rights in and to the Software
# and all intellectual property rights therein.  IF YOU HAVE
# NO AUTHORIZED LICENSE, THEN YOU HAVE NO RIGHT TO USE THIS SOFTWARE
# IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY BROADCOM AND DISCONTINUE
# ALL USE OF THE SOFTWARE.  
#  
# Except as expressly set forth in the Authorized License,
#  
# 1.     This program, including its structure, sequence and organization,
# constitutes the valuable trade secrets of Broadcom, and you shall use
# all reasonable efforts to protect the confidentiality thereof,
# and to use this information only in connection with your use of
# Broadcom integrated circuit products.
#  
# 2.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS
# PROVIDED "AS IS" AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES,
# REPRESENTATIONS OR WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY,
# OR OTHERWISE, WITH RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY
# DISCLAIMS ANY AND ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY,
# NONINFRINGEMENT, FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES,
# ACCURACY OR COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR
# CORRESPONDENCE TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING
# OUT OF USE OR PERFORMANCE OF THE SOFTWARE.
# 
# 3.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL
# BROADCOM OR ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL,
# INCIDENTAL, SPECIAL, INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER
# ARISING OUT OF OR IN ANY WAY RELATING TO YOUR USE OF OR INABILITY
# TO USE THE SOFTWARE EVEN IF BROADCOM HAS BEEN ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGES; OR (ii) ANY AMOUNT IN EXCESS OF
# THE AMOUNT ACTUALLY PAID FOR THE SOFTWARE ITSELF OR USD 1.00,
# WHICHEVER IS GREATER. THESE LIMITATIONS SHALL APPLY NOTWITHSTANDING
# ANY FAILURE OF ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.$
# 
################################################################################
#
# lmsc.pl
#
# Linux Module Symbol Checker
#
# Look for all undefined or non-exported symbols in the given set of files. 
# 


sub help
  {
    print "Linux Module Symbol Checker\n\n"; 
    print "  Check for undefined symbols between a group modules.\n";
    print "  Can be used to detect link errors which will occur at insmod time\n"; 
    print "  and verify the expected link dependencies between them.\n"; 
    print "  Any symbols which cannot be linked between the set of given files\n"; 
    print "  will be displayed.\n\n"; 
    print "Usage: lmsc.pl [option=value]* <file0> <file1> <file2> ...\n"; 
    print "       lmsc.pl [option=value]* <file0>,<file1>,<file2> <file1,file10> ..\n"; 
    print "  Options:\n"; 
    print "    kernel=<filename>    This image is included in all file sets.\n"; 
    print "    ferror=<num>         Specify exit code when files cannot be found.\n";
    print "                         Allows recovery in makefiles.\n";
    exit(2); 
  }
      
    
use File::Basename; 

#
# File aliases. Mainly used to reference kernel images
#
my $KERN_PATH="/usr/local/Cavium_Networks/OCTEON-SDK/linux/kernel/linux";
my $KERN_BASE="$KERN_PATH/vmlinux"; 
my $KERN_REL=`uname -r`; 

# Strip trailing whitespace
$KERN_REL =~ s/\s+$//;

my %ALIASES = ("nsx", "$KERN_BASE.nsx", 
               "nsx-2_6", "$KERN_BASE.nsx-2.6", 
               "nsx_wrl-2_6", "$KERN_BASE.nsx-2.6wrs", 
		"octeon-3_6", "$KERN_BASE.64",

               "gto", "$KERN_BASE.gto", 
               "gto-2_6", "$KERN_PATH/vmlinux.gto-2.6wrs", 
               "gto-2_6-wr30", "$KERN_PATH/vmlinux.gto-2.6wrs30", 
               "gto-2_6.2.6.21", "$KERN_PATH/vmlinux.gto-2_6.eldk.21",
               "gto-2_6.2.6.24", "$KERN_PATH/vmlinux.gto-2_6.eldk.24",
               "gto-2_6.2.6.25", "$KERN_PATH/vmlinux.gto-2_6.eldk.25",

               "gtx", "$KERN_BASE.gtx", 
               "gtx-2_6", "$KERN_PATH/vmlinux.gtx-2.6wrl", 
               
               "bmw-2_6", "$KERN_BASE.bmw-2.6wrs", 
               
               "jag", "$KERN_BASE.jag",
               "jag-2_6", "$KERN_BASE.jag-2.6wrs", 
               
               "raptor", "$KERN_BASE.raptor", 
               "raptor-2_6", "$KERN_BASE.raptor-2.6wrs",
               
               "robo-bsp-be", "$KERN_BASE.robo",
               "robo-bsp-2_6", "$KERN_BASE.robo-2.6wrs",
               "keystone-2_6", "$KERN_BASE.key_be-26wrs",
               "keystone_le-2_6", "$KERN_BASE.key_le-26wrs",
               "keystone-2_6-wr30", "$KERN_PATH/vmlinux.key_be-26wrs303",
               "keystone_le-2_6-wr30", "$KERN_PATH/vmlinux.key_le-26wrs303",
               "keystone-2_6.2.6.27", "$KERN_PATH/vmlinux.key-2_6.eldk.27",
               "keystone-2_6-highmem", "$KERN_PATH/vmlinux.key_be_highmem-26wrs",
               "keystone_le-2_6-highmem", "$KERN_PATH/vmlinux.key_le_highmem-26wrs",
               "keystone-2_6-wr30-highmem", "$KERN_PATH/vmlinux.key_be_highmem-26wrs303",
               "keystone_le-2_6-wr30-highmem", "$KERN_PATH/vmlinux.key_le_highmem-26wrs303",
               "keystone-2_6.2.6.27-highmem", "$KERN_PATH/vmlinux.key_highmem-2_6.eldk.27",
               "iproc-2_6", "$KERN_PATH/../ns/linux/kernel/linux/vmlinux",
               "x86-generic-2_6", "/lib/modules/$KERN_REL/source/vmlinux",
               "x86-generic_64-2_6", "/lib/modules/$KERN_REL/source/vmlinux",
               "x86-smp_generic-2_6", "/lib/modules/$KERN_REL/source/vmlinux",
               "x86-smp_generic_64-2_6", "/lib/modules/$KERN_REL/source/vmlinux",
              ); 
               
#
# All of the sets of files which require dependency/link checking
#
my @FILESETS; 

#
# All defined symbols, by file
my %DEFINED; 

#
# All undefined symbols, by file
#
my %UNDEFINED;


#
# Simple command line options
#
my @FILES; 
my $KERNEL; 
my $FERROR = -1; 

if(@ARGV==0) {
  help(); 
}

foreach (@ARGV) {
  if(/kernel=(.*)/) {
    # Specifies a kernel name that should be prepended to all sets
    $KERNEL = $1; 
    next;
  }
  if(/ferror=(.*)/) {
    # Override exit code if file does not exist
    $FERROR=$1; 
    next; 
  }
  if(/^(--help|-h|--h|help)/) {
    help(); 
  }

  push @FILES, $_; 
}

#
# A single set of files can be specified as "file0 file1 file2..." on the command line. 
# Multiple sets of files can be specified as "file0,file1,file2 file0,file2,file3..." on the command line. 
# 

if((grep { /,/ } @FILES) == 0) {
  # Assume all arguments are in the same fileset  
  push @FILESETS, join(",", @FILES); 
}
else {
  # Each argument represents a separate fileset
  push @FILESETS, @FILES;   
}

#
# Add the kernel image if specified
#
if(defined($KERNEL)) {
  map { $_ = "$KERNEL,$_"; } @FILESETS; 
}



#
# Load all file symbols
#
load_all_symbols(); 


#
# Check all filesets and set exit code
#
exit(check_all()); 


################################################################################

#
# Check for filename aliases
#
sub alias
  {
    my $file = shift; 
    return defined($ALIASES{$file}) ? $ALIASES{$file} : $file; 
  }
    

#
# Check All Filesets
#
sub check_all
  {
    my $count; 
    foreach my $fs (@FILESETS) {
      $count += check_fs(split /,/,$fs); 
    }
    return $count == 0 ? 0 : -1; 
  }

#
# Check a single fileset
#
sub check_fs
  {
    my @files = @_; 

    my @defined; 

    # Get all defined symbols for this fileset
    foreach (@files) {
      push @defined, @{$DEFINED{alias($_)}}; 
    }
    
    # Check 
    my @pfiles = map { basename($_); } @files; 
    printf("Link Check: @pfiles: "); 
    my $count = 0; 

    foreach my $file (@files) {
      foreach my $sym (@{$UNDEFINED{$file}}) {
        if($sym =~ /^__crc_(.+)/) {
          $sym = $1; 
        } 

        if((grep { $_ eq $sym } @defined) == 0) {
          printf("\n    *** %s: $sym", basename($file)); 
          $count++;
        }
      }
    }

    if($count == 0) {
      printf("OK"); 
    }
    printf("\n"); 
    return $count; 
  }


#
# Load all defined and undefined symbols for a file
#
sub load_all_symbols
  {    
    foreach my $fs (@FILESETS) {
      foreach my $file (split /,/, $fs) {
        
        #
        # Check for aliases
        #
        $file = alias($file); 
        
        # Load the defined symbols if we haven't already
        if(!defined($DEFINED{$file})) {

          #
          # File exist?
          #
          if(!(-e $file)) {
            printf("Warning: Cannot find file '$file'. Link check not performed.\n"); 
            exit($FERROR); 
          }

          #
          # Get all defined symbols
          #   
          my @defined = nm($file, "--defined-only"); 
    
          # 
          # If these are 2.6 modules, only the ksymtab symbols are actually available for linking. 
          #
          if(grep { /__ksymtab_/ } @defined) {
            @defined = grep { s/__ksymtab_// } @defined; 
          }
          
          $DEFINED{$file} = \@defined; 
        }

        # Load the undefined symbols if we haven't already
        if(!defined($UNDEFINED{$file})) {
          my @undefined = nm($file, "--undefined-only"); 
          $UNDEFINED{$file} = \@undefined;        
        }
      }
    }
  }

#
# Use 'nm' to retrieve symbols
#
sub nm
  {
    my ($file, @flags) = @_; 

    my @symbols; 
    
    foreach my $nm_output (`/tools/bin/nm @flags $file`) {
      my @lines = split(/\n/, $nm_output);
      foreach(@lines) {
        @str = split / / ;
        $symbol = $str[$#str];
        
        # Magic symbols
        if($symbol =~ /(__start|__stop)___kallsyms/) {
          next;
        }
        if($symbol =~ /__this_module/) {
          next;
        }
        if($symbol =~ /__stack_chk_/) {
          next;
        }
        if($symbol =~ /mcount/) {
          next;
        }
        push @symbols, $symbol;
      }
    }
    chomp @symbols;
    return @symbols;
  }

