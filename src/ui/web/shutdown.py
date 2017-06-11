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

class shutdown:

    def __init__(self):
      print "Shutdown";        
        
    @cherrypy.expose
    def index(self):  
      cherrypy.engine.exit();
      return        
