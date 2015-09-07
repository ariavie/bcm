#!/usr/bin/perl -w

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
# $Id$
#


use strict;
use warnings;
use File::Basename;
use IO::File;
use File::Copy;
use Data::Dumper;

my $arg;
my @files = ();
my $verbose = 0;
my $backup = 0;

while ( defined ( $arg = shift @ARGV) ) {
	if ( $arg eq '-h' || $arg eq '-help' ) {
        print_help();
		exit 0;
	}
	if ( $arg eq '-f' ) {
        while ( defined ( my $tmp = shift @ARGV ) ) {
            if ( $tmp !~ /^-/ ) {
                push ( @files, $tmp );
            } else {
                unshift ( @ARGV, $tmp );
                last;
            }
        }
        next;
    }
	if ( $arg eq '-v' ) {
		$verbose = 1;
		next;
	}
	if ( $arg eq '-b' ) {
		$backup = 1;
		next;
	}
    print "Illegal option $arg\n";
}

    foreach my $file ( @files ) {
		copy("$file", "$file.bak") or die "Copy failed: $!" if ( $backup );
        my $savedline;
        my $line;
        print "Processing file $file\n" if ($verbose );
        my $fhr = IO::File->new("$file",'r') || die "$! Unable to open file $file for reading\n";
        my $fhw = IO::File->new("$file.$$",'w') || die "$! Unable to open file $file.$$ for writing\n";
        while ( $line=<$fhr> ) {
            chomp ( $line );
# Delete one line so i can check in. 
			$line =~ s/[^!-~\s]//g; 
            if ( $line =~ s/\\$/ / ) {
                $savedline .= $line;
                next;
            }
            if ( defined $savedline ) { 
                $line =~ s/^\s+//;
                $line = $savedline . $line;
                undef $savedline;
            }
            print $fhw $line, "\n";
        }
        close ( $fhr );
        close ( $fhw );
        rename ( "$file.$$", "$file") || die "$! Unable to rename $file.$$ to $file\n" 
    }

sub print_help {
    print << "EOT"
$0 -f <files>

This script converts DOS lf/cr to UNIX format and remove all non-ASCII caharacters. It also joins lines seperated
by \ character into a single line. 

OPTIONS:
     -v             Enable verbose
     -b             Make backup of original files
     -h             help
     -help          help


USAGE:
$0 -f *.c *.h -v -b

EOT
}
