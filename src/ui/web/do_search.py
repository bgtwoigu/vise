#
# ==== Author:
# 
# Relja Arandjelovic (relja@robots.ox.ac.uk)
# Visual Geometry Group,
# Department of Engineering Science
# University of Oxford
#
# ==== Copyright:
#
# The library belongs to Relja Arandjelovic and the University of Oxford.
# No usage or redistribution is allowed without explicit permission.
#

import cherrypy;
import os.path;
import math;

from upload import savedTemp;

import uuid;
import json;


import get_scriptroot;
scriptroot= get_scriptroot.getScriptroot();
tmpDir=    os.path.join( scriptroot, 'tmp/' );


_username_key_= 'username';


class doSearch:
    
    
    
    def __init__(self, pageTemplate, API_obj, docMap, pathManager_obj, upload_obj= None, examples= None, guiOpts= None ):
        
        self.pT= pageTemplate;
        self.API_obj= API_obj;
        self.docMap= docMap;
        self.pathManager_obj= pathManager_obj;
        self.upload_obj= upload_obj;
        self.datasets= self.API_obj.keys();
        self.examples= examples;
        self.guiOpts= guiOpts;
        
        self.def_dsetname= self.docMap.keys()[0];
    
    @cherrypy.expose
    def index(self, docID= None, uploadID= None, xl= None, xu= None, yl= None, yu= None, startFrom= "1", numberToReturn= "20", tile= None, noText= None, dsetname= None):
        
        if dsetname==None: dsetname= self.def_dsetname;
        
        emailFeedback= False;
        if tile==None:
            tile= 'tile' in self.guiOpts['defaultView'][dsetname];
        else:
            tile= (tile!='false');
        showText= tile and noText==None;
        
        if xl!=None: xl= float(xl);
        if xu!=None: xu= float(xu);
        if yl!=None: yl= float(yl);
        if yu!=None: yu= float(yu);
        # if query regions are tiny: make them big
        if xl!=None and xu!=None and abs(xu-xl)<5:
            xl= None; xu= None;
        if yl!=None and yu!=None and abs(yu-yl)<5:
            yl= None; yu= None;
        startFrom= int(startFrom)-1;
        numberToReturn= int(numberToReturn);
        
        if xl==None: xl= 0;
        if yl==None: yl= 0;
        
        
        if uploadID==None:
            docID= int(docID);
            if xu==None or xu==None: imw, imh= self.docMap[dsetname].getWidthHeight(docID);
            if xu==None: xu= imw;
            if yu==None: yu= imh;
            queryFn= self.pathManager_obj[dsetname].hidePath(docID);
            results= self.API_obj[dsetname].internalQuery( docID= docID, xl= xl, xu= xu, yl= yl, yu= yu, startFrom= startFrom, numberToReturn= numberToReturn+1 );
        else:
            st= savedTemp.load(uploadID);
            queryFn= st['originalFullFilename'];
            if xu==None or yu==None:
                if xu==None: xu= st['w'];
                if yu==None: yu= st['h'];
            results= self.API_obj[dsetname].externalQuery( st['compDataFilename'], xl= xl, xu= xu, yl= yl, yu= yu, startFrom= startFrom, numberToReturn= numberToReturn+1 );
            del st;
        
        #temp
        first= startFrom;
        last= startFrom + len(results)-1;
        
        
        if uploadID==None:
            querySpec0= "docID=%d" % docID;
            querySpec1= "docID1=%d" % docID;
        else:
            querySpec0= "uploadID=%s" % uploadID;
            querySpec1= "uploadID1=%s" % uploadID;
        
        
        body= "";
        
        # switch between tiled and list views
        
        if tile:
            # show text option
            switchText= ''' |
                <a href="dosearch?%s&xl=%.2f&xu=%.2f&yl=%.2f&yu=%.2f&numberToReturn=%d&startFrom=%d&tile=%s&%s">
                %s text
                </a>''' % ( querySpec0, xl, xu, yl, yu, numberToReturn, startFrom+1, "true" if tile else "false", "noText" if showText else "", "No" if showText else "Show" );
        else:
            switchText= '';
        
        body+= '''
        <center>
        <table width="80%%" border="0">
        <tr><td align="right">
            <a href="dosearch?%s&xl=%.2f&xu=%.2f&yl=%.2f&yu=%.2f&numberToReturn=%d&startFrom=%d&tile=%s&%s">
            See %s view
            </a>
             %s
        </td></tr>
        </table>
        </center>''' % ( querySpec0, xl, xu, yl, yu, numberToReturn, startFrom+1, "false" if tile else "true", ("" if showText else "noText") if tile else "", "list" if tile else "tiled", switchText );
        
        # query image
        
        queryImage= '<a href="search?%s"><img width="200" src="getImage?%s&width=200&xl=%.2f&xu=%.2f&yl=%.2f&yu=%.2f&H=1,0,0,0,1,0,0,0,1&%s">' % (querySpec0, querySpec0, xl,xu,yl,yu, "crop" if tile else "");
        
        body+= "<h3>Query Image</h3>";
        body+= '<center><table width="80%" border="0">';
        body+= """
            <tr>
                <td>&nbsp;</td>
                <td>name: %s</td>
                <td width="210" align="center">%s</td>
                <td width="20" align="center">&nbsp;</td>
            </tr></table></center><br>
            """  % (queryFn, queryImage);
        
        
        # navigation
        
        if len(results)>numberToReturn:
            isLastPage= False;
            results=results[:numberToReturn];
        else:
            isLastPage= True;
        
        tileStr= "tile=true" if tile else "tile=false";
        showTextStr= "" if showText else "noText";
        navigation= "Navigation: ";
        noNavigation= True;
        if startFrom>0:
            noNavigation= False;
            if 1<startFrom+1-numberToReturn:
                navigation+= '<a href= "dosearch?%s&xl=%.2f&xu=%.2f&yl=%.2f&yu=%.2f&numberToReturn=%d&startFrom=1&%s&%s">First</a>' % (querySpec0,xl,xu,yl,yu,numberToReturn,tileStr,showTextStr);
                navigation+= "&nbsp;&nbsp;";
            navigation+= '<a href="dosearch?%s&xl=%.2f&xu=%.2f&yl=%.2f&yu=%.2f&numberToReturn=%d&startFrom=%d&%s&%s">Prev</a>' % (querySpec0,xl,xu,yl,yu,numberToReturn,max(1,startFrom+1-numberToReturn),tileStr,showTextStr);
        if startFrom>0 or not(isLastPage):
            noNavigation= False;
            navigation+= "&nbsp;&nbsp;";
        if not(isLastPage):
            noNavigation= False;
            navigation+= '<a href="dosearch?%s&xl=%.2f&xu=%.2f&yl=%.2f&yu=%.2f&numberToReturn=%d&startFrom=%d&%s&%s">Next</a>' % (querySpec0,xl,xu,yl,yu,numberToReturn,startFrom+1+numberToReturn,tileStr,showTextStr);
        if noNavigation:
            navigation= "";
        
        
        
        if not(tile):
            
            body+= "<h3>Search Results %d to %d</h3>" % (startFrom+1, startFrom+min(numberToReturn, len(results)) );
            body+= "<center><br>"+navigation+"</center><br><br>\n";
        
        else:
            
            body+= '<table width="100%" border="0"><tr><td align="left">';
            body+= "<h3>Search Results %d to %d</h3>" % (startFrom+1, startFrom+min(numberToReturn, len(results)) );
            body+= '</td><td align="right">'+navigation+"</td></tr></table><br>\n";
            
            #body+= "<h3>Search Results %d to %d</h3>" % (startFrom+1, startFrom+min(numberToReturn, len(results)) );
            #body+= '<div style="position:absolute;text-align:right;">' + navigation +'</div>\n';
        
        
        body+= '<center>';
        body+= '<table width="80%" border="0">';
        
        if not(tile):
            
            # results
            
            if emailFeedback: js_imageNames= [];
            
            for (rank, docIDres, score, H) in results:
                
                boxArg="xl=%.2f&xu=%.2f&yl=%.2f&yu=%.2f" % (xl,xu,yl,yu);
                if H!=None:
                    boxArg+= "&H=%s" % H;
                    detailedMatches= "<br><a href=\"details?%s&docID2=%d&%s\">Detailed matches</a><br>" % (querySpec1, docIDres, boxArg);
                else:
                    detailedMatches= "<br><a href=\"details?%s&docID2=%d&%s&drawPutative=true\">Detailed matches</a><br>" % (querySpec1, docIDres, boxArg);
                
                if emailFeedback:
                    tickChecked= "checked";
                    if uploadID==None and rank==0: tickChecked= "";
                    tickBox= '<input type="checkbox" id="tick_%d" %s onchange="javascript:updateTicked();">' % (rank, tickChecked);
                else:
                    tickBox= '';
                
                hiddenPath= self.pathManager_obj[dsetname].hidePath(docIDres);
                if emailFeedback: js_imageNames.append( hiddenPath );
                
                body+= """
                <tr>
                    <td>%d</td>
                    <td>
                        name: %s<br>
                        score: %.6f<br>
                        %s
                    </td>
                    <td width="210" align="center">
                        <a href="search?docID=%d">
                            <img src="getImage?docID=%s&width=200&%s">
                        </a>
                    </td>
                    <td width="20" align="center">
                        %s
                    </td>
                </tr>
                <tr><td colspan="4"><hr style="border:dashed; border-width:1px 0 0 0;"></td></tr>
                """ % (rank+1, hiddenPath, score, \
                       detailedMatches, \
                       docIDres, docIDres, boxArg , \
                       tickBox );
            
        else:
            
            infos= [];
            numPerRow= 4;
            
            for (rank, docIDres, score, H) in results:
                
                boxArg="xl=%.2f&xu=%.2f&yl=%.2f&yu=%.2f" % (xl,xu,yl,yu);
                if H!=None:
                    boxArg+= "&H=%s" % H;
                    detailedMatches= "<br><a href=\"details?%s&docID2=%d&%s\">Detailed matches</a><br>" % (querySpec1, docIDres, boxArg);
                else:
                    #break;
                    detailedMatches= "<br><a href=\"details?%s&docID2=%d&%s&drawPutative=true\">Detailed matches</a><br>" % (querySpec1, docIDres, boxArg);
                
                hiddenPath= self.pathManager_obj[dsetname].hidePath(docIDres);
                
                infos.append( (docIDres,score,boxArg,H!=None,hiddenPath,detailedMatches) );
                
                if len(infos)==numPerRow:
                    if showText:
                        body+='<tr>\n';
                        for (docIDres,score,boxArg,hasH,hiddenPath,detailedMatches) in infos:
                            body+= '<td align="center">%s</td>' % hiddenPath;
                        body+='</tr>';
                    body+='<tr>';
                    for (docIDres,score,boxArg,hasH,hiddenPath,detailedMatches) in infos:
                        body+= '''
                        <td align="center">
                        <a href="search?docID=%d">
                            <img src="getImage?docID=%s&width=160&%s&%s">
                        </a>
                        %s
                        </td>''' % (docIDres,docIDres,boxArg, "crop" if hasH else "", detailedMatches if showText else "");
                    body+='</tr>';
                    if showText:
                        body+='<tr><td colspan="%d"><hr style="border:dashed; border-width:1px 0 0 0;"></td></tr>\n' % numPerRow;
                    infos= [];
            
            if len(infos)>0:
                if showText:
                    body+='<tr>\n';
                    for (docIDres,score,boxArg,hasH,hiddenPath,detailedMatches) in infos:
                        body+= '<td align="center">%s</td>' % hiddenPath;
                    if len(infos)<numPerRow:
                        body+='<td colspan="%d"></td>' % (numPerRow-len(infos));
                    body+='</tr>';
                body+='<tr>';
                for (docIDres,score,boxArg,hasH,hiddenPath,detailedMatches) in infos:
                    body+= '''
                    <td align="center">
                    <a href="search?docID=%d">
                        <img src="getImage?docID=%s&width=160&%s&%s">
                    </a>
                    %s
                    </td>''' % (docIDres,docIDres,boxArg,"crop" if hasH else "", detailedMatches if showText else "");
                if len(infos)<numPerRow:
                    body+='<td colspan="%d"></td>' % (numPerRow-len(infos));
                body+='</tr>';
                infos= [];
                
           
            
        body+= "</table>";
        
        body+= "<br>"+navigation+"<br><br>\n";
        
        body+= "</center>";
        
        if emailFeedback:
            
            # send email with comments
            
            username= cherrypy.session.get( _username_key_, default= None );
            if username=='relja':
                toEmail= "relja@robots.ox.ac.uk";
            elif username=='giles':
                toEmail= "giles.bergel@merton.ox.ac.uk";
            elif username=='alex':
                toEmail= "Alexandra.Franklin@bodleian.ox.ac.uk";
            else:
                toEmail= "";
            
            body+= """

    <br><br>
    <center><h3>Please tick the images to be appended to the email.</h3></center><br>
    <form name="email" action="sendComments" method="post">
    <input type="hidden" name="queryRec" value=""" + \
('"%.2f,%.2f,%.2f,%.2f"' % (xl,xu,yl,yu) ) + \
'''>
    <table width="100%" cellspacing="2" border="0">
    
    <tr>
        <td>To:</td>
        <td><input type="text" value="''' +toEmail+ '''" style="width:100%" name="toemail"></td>
    </tr>
    
    <tr>
        <td>Subject:</td>
        <td><input type="text" value="Visual document matching" style="width:100%" name="subject"></td>
    </tr>
    
    <tr><td colspan="2">
    Message body<br>
    <textarea style="width:100%" rows="20" name="emailbody">
Comment
    </textarea>
    </td></tr>
    
    <tr><td colspan="2" align="center">
    <input type="submit" value="Send"">
    </td></tr>
    
    <tr><td colspan="2">
    Selected image names (to be appended to the email):
    </td></tr>
    
    <tr><td colspan="2">
    <textarea style="width:100%" rows="10" name="tickedList" id="tickedList">
    </textarea>
    </td></tr>
    
    </table>
    </form>
    
    <script language="javascript">
        updateTicked();
    </script>
    
    <br>

            ''';
            
            
            
            headExtra="""
    <script language="javascript">
    
    var first= %d;
    var last= %d;
    var imageNames=new Array("%s");
    
    function updateTicked(){
        var tickedList= 'Query image: '+imageNames[0]+"\\nResults:\\n";
        for (i=first; i<=last; ++i){
            if (document.getElementById('tick_'+i).checked) {
                tickedList+= imageNames[i]+"\\n";
            }
        }
        document.getElementById('tickedList').value= tickedList;
        return tickedList;
    }
    
    </script>
""" % (first, last, '","'.join(js_imageNames));
        
        
        
        else:
            
            headExtra= "";
        
        
        
        return self.pT.get(title= "Exact Matches", headExtra= headExtra, body= body);
        
