//
//  html_tags.h
//  AIPS
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by Alex Bettarini on 29 Apr 2019
//

#ifndef html_tags_h
#define html_tags_h

#define WITH_SEPARATE_TABLE_HEADER

#define COL_SPAN_L     "<col span=\""
#define COL_SPAN_R     "\" style=\"background-color: #EEEEEE; padding-right: 5px; padding-left: 5px\"/>"

#define TAG_TABLE_L     "<table class=\"s14\">"
#define TAG_TABLE_R     "</table>"
#define TAG_TD_L        "<td class=\"s13\"><p class=\"s11\">"
#define TAG_TD_R        "</p><div class=\"s12\"/></td>"

#ifdef WITH_SEPARATE_TABLE_HEADER
#define TAG_TH_L        "<th>"
#define TAG_TH_R        "</th>"
#else
#define TAG_TH_L        TAG_TD_L
#define TAG_TH_R        TAG_TD_R
#endif


#endif /* html_tags_h */
