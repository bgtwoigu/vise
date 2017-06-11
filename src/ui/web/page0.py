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

import random;

class page0:
    
    def __init__(self, pageTemplate, docMap, pathManager_obj, examples= None, externalExamples= None, browse= True, doShowPath= True):
        self.pT= pageTemplate;
        self.docMap= docMap;
        self.examples= examples;
        self.externalExamples= externalExamples;
        self.pathManager_obj= pathManager_obj;
        self.browse= browse;
        self.doShowPath= doShowPath;
        self.def_dsetname= self.docMap.keys()[0];
    
    @cherrypy.expose
    def index(self, page= '-1', more='false', numPerPage= "20", browse= None, dsetname= None ):
        
        if dsetname==None: dsetname= self.def_dsetname;
        
        if browse==None:
            browse= self.browse;
        else:
            browse= (browse=="true");
        page= int(page);
        seed= page;
        numPerPage= int(numPerPage);
        numPerRow= 4;
        
        total_num = len(self.docMap[dsetname]);
        
        if browse:
            
            if page<0:
                page= 0;
            startFrom= min(page*numPerPage,total_num-1);
            endBefore= min( (page+1)*numPerPage, total_num );
            sampleImages= range(startFrom,endBefore);
            lastPage= (total_num+numPerPage-1)/numPerPage-1;
            
            navigation= "<br>Images %d to %d of %d. " % (startFrom+1, endBefore, total_num);
            
            if page>0:
                if page>1:
                    navigation+= '<a href="page0?&numPerPage=%d&browse=true&page=%d">First</a>' % ( numPerPage, 0 );
                    navigation+= "&nbsp;&nbsp;";
                navigation+= '<a href="page0?&numPerPage=%d&browse=true&page=%d">Prev</a>' % ( numPerPage, page-1 );
            if page>0 and page<lastPage:
                navigation+= "&nbsp;&nbsp;";
            if page<lastPage:
                navigation+= '<a href="page0?numPerPage=%d&browse=true&page=%d">Next</a>' % ( numPerPage, page+1 );
            navigation+= "<br><br>\n";
            
        else:
            
            if seed==-1:
                random.seed();
            else:
                random.seed(seed);
            nextSeed= random.randint(0, 100000);
            
            sampleImages= [];
            for i in range(0,numPerPage):
                sampleImages.append( random.randint(0,total_num-1) );
        
        
        body= "";
        
        if self.examples and self.examples[dsetname]:
            body+= "<h3>Representative examples of images in the database</h3>";
            body+= "<center>";
            for example in self.examples[dsetname]:
                if type(example)==int:
                    docID= example;
                else:
                    docID= self.docMap[dsetname].getDocID( example );
                body+= '<a href="search?docID=%d"> <img width="160" src="getImage?docID=%d&width=160" alt="Image %d"> </a>\n' % (docID, docID, docID);
                #body+= '<img width="160" onclick=\'javascript:goto("search?docID=%d")\' src="getImage?docID=%d&width=160" alt="Image %d" style="cursor:pointer;">\n' % (docID, docID, docID);
            body+="<br><br>\n";
            body+= "</center>";
        
        
        if self.externalExamples and self.externalExamples[dsetname]:
            formCounter= 0;
            formHtml= '<form action="upload?" method="post" name="upload%d" enctype="multipart/form-data"><input type="hidden" name="uploadURL" value="%s"></form>';
            body+= "<h3>Examples of external images</h3>";
            body+= "<center>";
            for externalExample in self.externalExamples[dsetname]:
                body+= formHtml % (formCounter, externalExample);
                body+= '<a href="javascript:document.upload%d.submit();"> <img width="160" src="%s"> </a>\n' % (formCounter, externalExample);
                formCounter+= 1;
            body+="<br><br>\n";
            body+= "</center>";
            
        
        if browse:
            body+= """<h3>List of images in the database</h3>""";
        else:
            #body+= """<h3>Random samples of images in the database (click <a href="javascript:location.reload(true)">here</a> for another one)</h3>""";
            body+= '<a name="samples"></a>';
            body+= """<h3>Random samples of images in the database (click <a href="page0?page=%d&browse=false#samples">here</a> for another one)</h3>""" % nextSeed;
        
        if browse:
            body+= navigation;
        
        body+= "<center>"
        body+='<table width="100%">\n';
        iRowImage= 0; iImage= 0;
                
        for docID in sampleImages:
            if iRowImage==0:
                body+='<tr>\n';
                for jImage in range(0,numPerRow):
                    if iImage+jImage >= len(sampleImages):
                        body+= '<td colspan="%d"></td>' % (numPerRow-jImage);
                        break;
                    body+= '<td align="center">%s</td>' % ( self.pathManager_obj[dsetname].displayPath(sampleImages[iImage+jImage]) if self.doShowPath else "");
                body+='</tr>\n';
                body+="<tr>\n";
            
            body+='<td align="center">';
            body+= '<a href="search?docID=%d"> <img width="160" src="getImage?docID=%d&width=160" alt="Image %d"> </a>\n' % (docID, docID, docID);
            #body+= '<img width="160" onclick=\'javascript:goto("search?docID=%d")\' src="getImage?docID=%d&width=160" alt="Image %d" style="cursor:pointer;">\n' % (docID, docID, docID);
            body+="</td>";
            
            iRowImage+= 1;
            iImage+= 1;
            
            if (iRowImage%numPerRow==0):
                iRowImage= 0;
                body+="</tr>\n";
                body+='<tr><td colspan="%d"><hr style="border:dashed; border-width:1px 0 0 0;"></td></tr>\n' % numPerRow;
            
        if iRowImage!=0:
            body+='<td colspan="%d"></td></tr>\n' % (numPerRow-iRowImage);
        body+="</table>\n";
        body+= "</center>";
        if browse:
            body+= navigation;
        
        headExtra= """
    <script language="javascript">
        function goto(url){
            document.location.href= url;
        }
    </script>
        """;
        
        return self.pT.get( title= "Browse", body= body, headExtra= headExtra );
        
