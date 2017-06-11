var VISE_THEME_MESSAGE_TIMEOUT_MS = 4000;

var _vise_message_clear_timer;

// XHR
var _vise_server = new XMLHttpRequest();
var _vise_messenger = new XMLHttpRequest();
var _vise_query = new XMLHttpRequest();

var VISE_SERVER_ADDRESS    = "http://localhost:9971/";
var VISE_MESSENGER_ADDRESS = VISE_SERVER_ADDRESS + "_message";
var VISE_QUERY_ADDRESS     = VISE_SERVER_ADDRESS + "_query";

_vise_server.addEventListener("load", _vise_server_response_listener);
_vise_messenger.addEventListener("load", _vise_message_listener);
_vise_query.addEventListener("load", _vise_query_listener);

var _vise_current_state_id = -1;
var _vise_current_state_name = "";
var _vise_current_search_engine_name = "";
var _vise_search_engine_state;

// Image comparator
var _vise_query_width;
var _vise_query_height;

var _vise_imcomp_im1;
var _vise_imcomp_im2;
var im1_loaded = false;
var im2_loaded = false;

// Image region selector
var _vise_img_canvas;
var _vise_img_ctx;
var _vise_canvas_width, _vise_canvas_height;
var _vise_current_image_width;
var _vise_current_image_height;
var _vise_current_image;
var _vise_current_img_fn;
var _vise_canvas_scale = 1.0;

var _vise_click_x0;
var _vise_click_y0;
var _vise_click_x1;
var _vise_click_y1;
var _vise_current_x;
var _vise_current_y;

// state
var _vise_is_user_drawing_region = false;

// theme
var VISE_THEME_REGION_BOUNDARY_WIDTH = 4;
var VISE_THEME_BOUNDARY_FILL_COLOR   = "#aaeeff";
var VISE_THEME_BOUNDARY_LINE_COLOR   = "#1a1a1a";

// html elements
var header = document.getElementById('header');
var control_panel = document.getElementById('control_panel');
var canvas_panel;

// image region
var original_img_region = new ImageRegion();
var canvas_img_region = new ImageRegion();

// experimental features
var is_exp_features_enabled = false;

function _vise_init() {
  // initially hide footer and log
  document.getElementById("footer").style.display = "none";
  document.getElementById("log").style.display = "none";

  /*
  // @todo allow graceful shutdown of VISE server
  window.onbeforeunload = function(e) {
    _vise_server.open("POST", VISE_SERVER_ADDRESS);
    _vise_server.send("shutdown_vise now");
  }
  */

  /**/
  // debug
  //_vise_current_search_engine_name = 'ox5k';
  //imcomp("christ_church_000212.jpg","christ_church_000333.jpg","x0y0x1y1=436,28,612,346","H=1.09694,0,6.49256,0.0291605,0.981047,-30.7954,0,0,1");

  //_vise_select_img_region( 'https://www.nasa.gov/sites/default/files/thumbnails/image/earthsun20170412.png' );

  // request the contents of vise_index.html
  _vise_server.open("GET", VISE_SERVER_ADDRESS + "_vise_home.html");
  _vise_server.send();

  // create the seed connection to receive messages
  _vise_messenger.open("GET", VISE_MESSENGER_ADDRESS);
  _vise_messenger.send();

}


//
// Vise Server ( localhost:9973 )
//
function _vise_server_response_listener() {
  var response_str = this.responseText;
  var content_type = this.getResponseHeader('Content-Type')

  if ( content_type.includes("text/html") ) {
    document.getElementById("content").innerHTML = response_str;
  } else if ( content_type.includes("application/json") ) {
    var json = JSON.parse(response_str);
    switch(json.id) {
      case 'search_engine_state':
        _vise_update_state_info(json);
        break;

      case 'http_post_response':
        // @todo handle post response, current assumption is that it will be OK
        // json['result'] === "OK"
        break;

      case 'dired':
        _vise_handle_dired_response(json);
        break;

      default:
        console.log("Do not know where to forward the received response!");
        console.log(response_str);
    }
  } else {
    console.log("Received response of unknown content-type : " + content_type);
  }
}

function _vise_message_listener() {
  var packet = this.responseText;

  // create a connection for next message
  _vise_messenger.open("GET", VISE_MESSENGER_ADDRESS);
  _vise_messenger.send();

  // process the received message asynchronously
  setTimeout( function() {
    _vise_handle_message( packet );
  }, 0);
}
// msg = sender receiver msg
function _vise_handle_message(packet) {
  //console.log("message: " + packet);
  var first_space  = packet.indexOf(' ', 0);
  var second_space = packet.indexOf(' ', first_space + 1);

  var sender   = packet.substr(0, first_space);
  var receiver = packet.substr(first_space + 1, second_space - first_space - 1);
  var msg = packet.substr(second_space + 1);

  if ( receiver === "command" ) {
    _vise_handle_command( sender, msg );
  } else if ( receiver === "log" ) {
    _vise_handle_log_message(sender, msg);
  } else if ( receiver === "message" ) {
    _vise_show_message(msg, VISE_THEME_MESSAGE_TIMEOUT_MS);
  } else if ( receiver === "progress" ) {
    _vise_handle_progress_message(sender, msg);
  }
}

function _vise_show_message(msg, t) {
  if ( _vise_message_clear_timer ) {
    clearTimeout(_vise_message_clear_timer); // stop any previous timeouts
  }
  document.getElementById('message_panel').innerHTML = msg;

  var timeout = t;
  if ( timeout > 0 || typeof t === 'undefined') {
    if ( typeof t === 'undefined') {
      timeout = VISE_THEME_MESSAGE_TIMEOUT_MS;
    }
    _vise_message_clear_timer = setTimeout( function() {
        document.getElementById('message_panel').innerHTML = ' ';
    }, timeout);
  }
}

//
// progress bar
//
function _vise_handle_progress_message(state_name, msg) {
  if ( msg.includes('/') ) {
    var values = msg.split('/');
    var completed = parseInt(values[0]);
    var total = parseInt(values[1]);
    var progress = Math.round( (completed/total) * 100 );
    //console.log( "Progress " + completed + " of " + total );

    document.getElementById("progress_bar").style.width = progress + "%";
    document.getElementById("progress_text").innerHTML = state_name + " : " + completed + " of " + total;

    var percent = parseInt( (completed / total ) * 100 );
    console.log("_vise_current_state_name = " + _vise_current_state_name + " : percent = " + percent);
    switch ( _vise_current_state_name ) {
      case "Preprocess": // descriptor
        if ( percent % 25 < 2 ) {
          _vise_fetch_random_image();
        } 
        break;

      case "Stage-1": // descriptor
        if ( percent % 10 < 2 ) {
          _vise_fetch_random_image();
        } 
        break;
      case "Stage-2": // cluster
        _vise_fetch_random_image();
        break;
      case "Stage-5":
        if ( percent % 4 < 1 ) {
          _vise_fetch_random_image();
        } 
        break;
    }
  } else {
    document.getElementById("progress_text").innerHTML = msg;
  }
}

function _vise_reset_progress_bar() {
  document.getElementById("progress_bar").style.width = "0%";
  document.getElementById("progress_text").innerHTML = "";
}

function _vise_complete_progress_bar() {
  var progress_bar = document.getElementById("progress_bar");
  progress_bar.style.width = "100%";
}

function _vise_handle_log_message(sender, msg) {
  var log_panel = document.getElementById( "log" );
  if ( msg.startsWith("\n") ) {
    msg = "\n" + sender + " : " + msg.substr(1);
  }
  if ( typeof log_panel !== 'undefined' ) {
    log_panel.innerHTML += msg;
    log_panel.scrollTop = log_panel.scrollHeight; // automatically scroll
  } 
}

//
// command
//
function _vise_handle_command(sender, command_str) {
  //console.log("sender = " + sender + " : command_str = " + command_str);
  // command_str = "_state update_now"
  // command_str = "_control_panel remove Info_proceed_button"
  var cmd = command_str;
  var param;
  if ( command_str.includes(' ') ) {
    var first_spc = command_str.indexOf(' ', 0);
    cmd = command_str.substr(0, first_spc);
    param = command_str.substr(first_spc+1);
  }

  switch ( cmd ) {
    case "_state":
      switch( param ) {
        case 'update_now':
          _vise_server_send_get_request("_state");
          break;
        case "show":
          document.getElementById("footer").style.display = "block";
          break;
        case "hide":
          document.getElementById("footer").style.display = "none";
          break;
      }
      break;

    case "_content":
      switch( param ) {
        case "clear":
          document.getElementById("content").innerHTML = "";
      }
      break;

    case "_log":
      _vise_handle_log_command(param);
      break;
    case "_control_panel":
      _vise_handle_control_panel_command(param);
      break;

    case "_dired":
      if ( param.startsWith("fetch") ) {
        var param_split = param.split(' ');
        _dired_current_path_name = param_split[1];
        _vise_server.open("GET", VISE_SERVER_ADDRESS + "_dired?path=" + _dired_current_path_name + "&folder=.");
        _vise_server.send();
      }
      break;

    case "_progress":
      var all_param = param.split(' ');
      for ( var i=0; i<all_param.length; i++ ) {
        switch( all_param[i] ) {
          case 'show':
            document.getElementById("progress_box").style.display = "block";
            break;
          case 'hide':
            document.getElementById("progress_box").style.display = "none";
            break;
          case 'reset':
            _vise_reset_progress_bar();
            break;
          case 'complete':
            _vise_complete_progress_bar();
            break;
        }
      }
      break;

    case "_redirect":
      var args = param.split(' ');
      if (args.length == 2) {
        var uri = args[0];
        var delay = parseInt( args[1] );
        setTimeout( function() {
          //var win = window.open( uri );
          //win.focus();
          window.location.href = uri;
        }, delay);
      } else {
        //var win = window.open( param );
        //win.focus();
        window.location.href = uri;
      }
      break;

    case "_go_to":
      switch( param ) {
        case "home":
          window.location.href = VISE_SERVER_ADDRESS;
          break;
      }
      break;

    case "_go_home":
      // request the contents of vise_index.html
      _vise_server.open("GET", VISE_SERVER_ADDRESS + "_vise_home.html");
      _vise_server.send();
      break;

    default:
      console.log("Unknown command : [" + command_str + "]");
  }
}

function _vise_handle_log_command(command_str) {
  var command = command_str.split(' ');
  for ( var i=0; i < command.length; i++ ) {
    switch( command[i] ) {
      case "show":
        document.getElementById("log").style.display = "block";
        break;
      case "hide":
        document.getElementById("log").style.display = "none";
        break;
      case "clear":
        document.getElementById("log").innerHTML = "&nbsp;";
        break;
      default:
        console.log("Received unknown log command : " + command[i]);
    }
  }
}

function _vise_toggle_log() {
  var log = document.getElementById("log");
  if ( log.style.display === 'undefined' || log.style.display === 'none' ) {
    document.getElementById("log").style.display = "block";
  } else {
    document.getElementById("log").style.display = "none";
  }
}

function _vise_handle_control_panel_command(command) {
  // command_str = "remove Info_proceed_button"
  var first_spc = command.indexOf(' ', 0);
  var cmd = command.substr(0, first_spc);
  var param = command.substr(first_spc+1);

  switch ( cmd ) {
    case "add":
      var cpanel = document.getElementById("control_panel");
      cpanel.innerHTML += param;
      break;
    case "remove":
      document.getElementById(param).remove();
      break;
    case "clear":
      if ( param === "all" ) {
        var cpanel = document.getElementById("control_panel");
        cpanel.innerHTML = "";
      }
      break;
  }
}

//
// Maintainer of UI to reflect VISE current state 
//
function _vise_update_state_info( json ) {
  var html = [];
  for ( var i=1; i<json['state_name_list'].length; i++ ) {
    html.push('<div class="state_block">');
    if ( i === json['current_state_id'] ) {
      html.push('<div class="title current_state">' + json['state_name_list'][i] + '</div>');
    } else {
      html.push('<div class="title">' + json['state_name_list'][i] + '</div>');
    }
    if ( json['state_info_list'][i] === '' ) {
      html.push('<div class="info">&nbsp;</div>');
    } else {
      html.push('<div class="info">' + json['state_info_list'][i] + '</div>');
    }
    html.push('</div>');
    html.push('<div class="state_block_sep">&rarr;</div>');
  }
  // remove the last arrow
  html.splice(html.length - 1, 1);
  document.getElementById("footer").innerHTML = html.join('');

  if ( _vise_current_state_id !== json['current_state_id'] ) {
    _vise_current_state_id = json['current_state_id'];
    _vise_current_state_name = json['state_name_list'][_vise_current_state_id];
    _vise_current_search_engine_name = json['search_engine_name'];
    _vise_search_engine_state = json;

    // request content for this state
    _vise_fetch_current_state_content();

    // reset the progress bar
    _vise_reset_progress_bar();

    if ( _vise_current_state_id > 2 ) {
      _vise_fetch_random_image();
    }

    // update browser title
    document.title = _vise_current_search_engine_name + " : VISE";
  }
}

function _vise_fetch_current_state_content() {
  //var resource_uri = _vise_current_search_engine_name + "/" + _vise_current_state_name;
  var resource_uri = _vise_current_state_name;
  _vise_server_send_get_request( resource_uri );
}
//
// Vise Server requests at
// http://localhost:8080
//
function _vise_create_search_engine() {
  var search_engine_name = document.getElementById('vise_search_engine_name').value;
  _vise_server_send_post_request("create_search_engine " + search_engine_name);
}

function _vise_delete_search_engine() {
  var search_engine_name = document.getElementById('vise_search_engine_name').value;
  var confirm_message = "Are you sure you want to delete \"" + search_engine_name + "\"?\nWARNING: deleted search engine cannot be recovered!";
  var confirm_delete = confirm(confirm_message);
  if ( confirm_delete ) {
    _vise_server_send_post_request("delete_search_engine " + search_engine_name);
  }
}

function _vise_load_search_engine(name) {
    _vise_server_send_post_request("load_search_engine " + search_engine_name);
}

function _vise_toggle_experimental_features() {
  if ( is_exp_features_enabled ) {
    var confirm_message = "Are you sure you want to enable experimental features of VISE?\nWARNING: these features may be unstable and may result in loss of data!";
    var confirm_delete = confirm(confirm_message);
    if ( confirm_delete ) {
      _vise_server_send_post_request("experimental_features 1");
      document.body.style.background = "#FFD5D5";
      is_exp_features_enabled = true;
    }
  } else {
      _vise_server_send_post_request("experimental_features 0");
      document.body.style.background = "#FFFFFF";
    is_exp_features_enabled = false;
  }
}

function _vise_server_send_post_request(post_data) {
  _vise_server.open("POST", VISE_SERVER_ADDRESS);
  _vise_server.send(post_data);
}

function _vise_server_send_state_post_request(state_name, post_data) {
  _vise_server.open("POST", VISE_SERVER_ADDRESS + state_name);
  _vise_server.send(post_data);
}

function _vise_server_send_get_request(resource_name) {
  _vise_server.open("GET", VISE_SERVER_ADDRESS + resource_name);
  _vise_server.send();
}

function _vise_send_msg_to_training_process(msg) {
  _vise_server_send_post_request("msg_to_training_process " + msg);
}

function _vise_fetch_random_image() {
  _vise_server.open("GET", VISE_SERVER_ADDRESS + "_random_image");
  _vise_server.send();
}

//
// State: Setting
//
function _vise_send_setting_data() {
  var postdata = [];
  var vise_basic_settings = document.getElementById("vise_basic_setting");
  if ( typeof vise_basic_settings !== 'undefined' && vise_basic_settings !== null ) {
    var setting_param = document.getElementsByClassName("vise_setting_param");

    for ( var i = 0; i < setting_param.length; i++) {
      var param_name  = setting_param[i].name;
      var param_value = setting_param[i].value;
      postdata.push( param_name + "=" + param_value );
    }
    var postdata_str = postdata.join('\n');
    _vise_server_send_state_post_request( _vise_current_state_name, postdata_str );
  }
}

//
// State: Query
//
function q(s) {
  // s = "cmd=show_img_list&arg1=value1&arg2=value2"
  _vise_query.open( "GET", VISE_QUERY_ADDRESS + "?" + s )
  _vise_query.send();
}

function _vise_query_listener() {
  var response_str = this.responseText;
  var content_type = this.getResponseHeader('Content-Type')

  if ( content_type.includes("text/html") ) {
    document.getElementById("content").innerHTML = response_str;
  } else if ( content_type.includes("application/json") ) {
    var json = JSON.parse(response_str);
    switch(json.id) {
      case '':
        break;

      default:
        console.log("Do not know where to forward the received query response!");
        console.log(response_str);
    }
  } else {
    console.log("Received response of unknown content-type : " + content_type);
  }
}

function ImageRegion() {
    this.is_user_selected  = false;
    this.shape_attributes  = new Map(); // region shape attributes
}

//
// Image region selector
//

function _vise_select_img_region(img_uri) {
  _vise_current_img_fn = img_uri;
  document.getElementById("content").innerHTML = '<div style="text-align: center;" id="vise_canvas_panel"><canvas id="_vise_img_canvas"></canvas><p style="display: inline;">Click and drag to draw a region</div>';
  _vise_img_canvas = document.getElementById('_vise_img_canvas');
  _vise_img_ctx = _vise_img_canvas.getContext('2d');

  canvas_panel = document.getElementById('vise_canvas_panel');
  _vise_load_canvas_img( _vise_current_img_fn );

  // add search button to control panel
  control_panel.innerHTML = '<div class="action_button" onclick="vise_search_img_region()">Search</div>';

  _vise_img_canvas.addEventListener('mousedown', _vise_canvas_mousedown_listener);
  _vise_img_canvas.addEventListener('mouseup'  , _vise_canvas_mouseup_listener);
  _vise_img_canvas.addEventListener('mousemove'  , _vise_canvas_mousemove_listener);
  _vise_img_canvas.addEventListener('mouseover'  , _vise_canvas_mouseover_listener);
}

function vise_search_img_region() {
  console.log(original_img_region.shape_attributes);
  console.log(canvas_img_region.shape_attributes);

  var attr = original_img_region.shape_attributes;
  if ( attr.size !== 0 ) {
    var x0 = parseInt( attr.get('x') );
    var y0 = parseInt( attr.get('y') );
    var width = parseInt( attr.get('width') );
    var height = parseInt( attr.get('height') );
    var x1 = x0 + width;
    var y1 = y0 + height;

    var query = [];
    query.push( "cmd=search_img_region" );
    query.push( "img_fn=" + _vise_current_img_fn );
    query.push( "x0y0x1y1=" + x0 + "," + y0 + "," + x1 + "," + y1 );
    console.log('Sending query ' + query.join('&') );
    q( query.join('&') );
  } else {
    console.log('Draw region first!' );
  }
}

function _vise_load_canvas_img(img_uri) {
  _vise_current_image = new Image();

  _vise_current_image.addEventListener( "error", function() {
      _vise_is_loading_current_image = false;
      show_message("Error loading image ]" + img_uri + "] !");
  }, false);

  _vise_current_image.addEventListener( "abort", function() {
      _vise_is_loading_current_image = false;
      show_message("Aborted loading image [" + img_uri + "] !");
  }, false);

  _vise_current_image.addEventListener( "load", function() {
      // update the current state of application
      _vise_current_image_loaded = true;
      _vise_is_loading_current_image = false;

      _vise_current_image_width = _vise_current_image.naturalWidth;
      _vise_current_image_height = _vise_current_image.naturalHeight;

      // set the size of canvas
      // based on the current dimension of browser window
      var de = document.documentElement;
      canvas_panel_width = de.clientWidth - 230;
      canvas_panel_height = de.clientHeight - 2*header.offsetHeight;
      _vise_canvas_width = _vise_current_image_width;
      _vise_canvas_height = _vise_current_image_height;
      var scale_width, scale_height;
      if ( _vise_canvas_width > canvas_panel_width ) {
          // resize image to match the panel width
          var scale_width = canvas_panel_width / _vise_current_image.naturalWidth;
          _vise_canvas_width = canvas_panel_width;
          _vise_canvas_height = _vise_current_image.naturalHeight * scale_width;
      }
      if ( _vise_canvas_height > canvas_panel_height ) {
          // resize further image if its height is larger than the image panel
          var scale_height = canvas_panel_height / _vise_canvas_height;
          _vise_canvas_height = canvas_panel_height;
          _vise_canvas_width = _vise_canvas_width * scale_height;
      }
      _vise_canvas_width = Math.round(_vise_canvas_width);
      _vise_canvas_height = Math.round(_vise_canvas_height);
      _vise_canvas_scale = _vise_current_image.naturalWidth / _vise_canvas_width;
      _vise_canvas_scale_without_zoom = _vise_canvas_scale;
      set_all_canvas_size(_vise_canvas_width, _vise_canvas_height);
      //set_all_canvas_scale(_vise_canvas_scale_without_zoom);

      // we only need to draw the image once in the image_canvas
      _vise_img_ctx.clearRect(0, 0, _vise_canvas_width, _vise_canvas_height);
      _vise_img_ctx.drawImage(_vise_current_image, 0, 0,
                             _vise_canvas_width, _vise_canvas_height);
  });
  _vise_current_image.src = img_uri;
}


function set_all_canvas_size(w, h) {
    _vise_img_canvas.height = h;
    _vise_img_canvas.width  = w;

    canvas_panel.style.height = h + 'px';
    canvas_panel.style.width  = w + 'px';
}

//
// mouse handling for drawing regions
//

function _vise_canvas_mouseover_listener(e) {
  document.getElementById('vise_canvas_panel').style.cursor = 'crosshair';
}

// user clicks on the canvas
function _vise_canvas_mousedown_listener(e) {
    _vise_click_x0 = e.offsetX; _vise_click_y0 = e.offsetY;
    _vise_is_user_drawing_region = true;
    e.preventDefault();
}

function _vise_canvas_mouseup_listener(e) {
  _vise_click_x1 = e.offsetX; _vise_click_y1 = e.offsetY;

  var click_dx = Math.abs(_vise_click_x1 - _vise_click_x0);
  var click_dy = Math.abs(_vise_click_y1 - _vise_click_y0);
  if ( _vise_is_user_drawing_region ) {
    _vise_is_user_drawing_region = false;

    var region_x0, region_y0, region_x1, region_y1;
    // ensure that (x0,y0) is top-left and (x1,y1) is bottom-right
    if ( _vise_click_x0 < _vise_click_x1 ) {
        region_x0 = _vise_click_x0;
        region_x1 = _vise_click_x1;
    } else {
        region_x0 = _vise_click_x1;
        region_x1 = _vise_click_x0;
    }

    if ( _vise_click_y0 < _vise_click_y1 ) {
        region_y0 = _vise_click_y0;
        region_y1 = _vise_click_y1;
    } else {
        region_y0 = _vise_click_y1;
        region_y1 = _vise_click_y0;
    }

    var region_dx = Math.abs(region_x1 - region_x0);
    var region_dy = Math.abs(region_y1 - region_y0);

    original_img_region.shape_attributes.set('name', 'rect');
    original_img_region.shape_attributes.set('x', Math.round(region_x0 * _vise_canvas_scale));
    original_img_region.shape_attributes.set('y', Math.round(region_y0 * _vise_canvas_scale));
    original_img_region.shape_attributes.set('width', Math.round(region_dx * _vise_canvas_scale));
    original_img_region.shape_attributes.set('height', Math.round(region_dy * _vise_canvas_scale));

    canvas_img_region.shape_attributes.set('name', 'rect');
    canvas_img_region.shape_attributes.set('x', Math.round(region_x0));
    canvas_img_region.shape_attributes.set('y', Math.round(region_y0));
    canvas_img_region.shape_attributes.set('width', Math.round(region_dx));
    canvas_img_region.shape_attributes.set('height', Math.round(region_dy));

    _vise_draw_region();
  }
  e.preventDefault();
}

function _vise_canvas_mousemove_listener(e) {
  if ( _vise_is_user_drawing_region ) {
    _vise_current_x = e.offsetX; _vise_current_y = e.offsetY;
    var region_x0, region_y0;

    if ( _vise_click_x0 < _vise_current_x ) {
        if ( _vise_click_y0 < _vise_current_y ) {
            region_x0 = _vise_click_x0;
            region_y0 = _vise_click_y0;
        } else {
            region_x0 = _vise_click_x0;
            region_y0 = _vise_current_y;
        }
    } else {
        if ( _vise_click_y0 < _vise_current_y ) {
            region_x0 = _vise_current_x;
            region_y0 = _vise_click_y0;
        } else {
            region_x0 = _vise_current_x;
            region_y0 = _vise_current_y;
        }
    }
    var dx = Math.round(Math.abs(_vise_current_x - _vise_click_x0));
    var dy = Math.round(Math.abs(_vise_current_y - _vise_click_y0));

    _vise_draw_region();
    _vise_draw_rect_region(region_x0, region_y0, dx, dy);
  }
}

function _vise_draw_region() {
  var attr = canvas_img_region.shape_attributes;

  _vise_img_ctx.clearRect(0, 0, _vise_canvas_width, _vise_canvas_height);
  _vise_img_ctx.drawImage(_vise_current_image, 0, 0,
                         _vise_canvas_width, _vise_canvas_height);

  _vise_draw_rect_region(attr.get('x'),
                         attr.get('y'),
                         attr.get('width'),
                         attr.get('height'));
}

function _vise_draw_rect_region(x, y, w, h) {
  // draw a fill line
  _vise_img_ctx.strokeStyle = VISE_THEME_BOUNDARY_FILL_COLOR;
  _vise_img_ctx.lineWidth   = VISE_THEME_REGION_BOUNDARY_WIDTH/2;
  _vise_draw_rect(x, y, w, h);
  _vise_img_ctx.stroke();

  if ( w > VISE_THEME_REGION_BOUNDARY_WIDTH &&
       h > VISE_THEME_REGION_BOUNDARY_WIDTH ) {
      // draw a boundary line on both sides of the fill line
      _vise_img_ctx.strokeStyle = VISE_THEME_BOUNDARY_LINE_COLOR;
      _vise_img_ctx.lineWidth   = VISE_THEME_REGION_BOUNDARY_WIDTH/4;
      _vise_draw_rect(x - VISE_THEME_REGION_BOUNDARY_WIDTH/2,
                     y - VISE_THEME_REGION_BOUNDARY_WIDTH/2,
                     w + VISE_THEME_REGION_BOUNDARY_WIDTH,
                     h + VISE_THEME_REGION_BOUNDARY_WIDTH);
      _vise_img_ctx.stroke();

      _vise_draw_rect(x + VISE_THEME_REGION_BOUNDARY_WIDTH/2,
                     y + VISE_THEME_REGION_BOUNDARY_WIDTH/2,
                     w - VISE_THEME_REGION_BOUNDARY_WIDTH,
                     h - VISE_THEME_REGION_BOUNDARY_WIDTH);
      _vise_img_ctx.stroke();
  }
}

function _vise_draw_rect(x, y, w, h) {
    _vise_img_ctx.beginPath();
    _vise_img_ctx.moveTo(x  , y);
    _vise_img_ctx.lineTo(x+w, y);
    _vise_img_ctx.lineTo(x+w, y+h);
    _vise_img_ctx.lineTo(x  , y+h);
    _vise_img_ctx.closePath();
}

//
// Image comparator
//

function imcomp(im1fn, im2fn, region, H) {
  var control_panel = document.getElementById('control_panel');
  control_panel.innerHTML = '';

  // extract the width and height of query region
  var region_csv = region.split('=')[1];
  var region_val = region_csv.split(',');
  var x0 = parseInt( region_val[0] );
  var y0 = parseInt( region_val[1] );
  var x1 = parseInt( region_val[2] );
  var y1 = parseInt( region_val[3] );
  var _vise_query_width  = x1 - x0;
  var _vise_query_height = y1 - y0;

  var content = document.getElementById('content');
  var html = [];
  html.push( '<ul class="img_list columns-3">' );

  var im1_uri = [];
  im1_uri.push( "/_static/" + _vise_current_search_engine_name + "/" + im1fn );
  im1_uri.push( "?crop=false" );
  im1_uri.push( "&scale=false");
  im1_uri.push( "&draw_region=true");
  im1_uri.push( "&" + region);
  html.push( '<li><h3>Query Image</h3><img src="' + im1_uri.join('') + '" /><p>' + im1fn + '</p></li>');

  html.push( '<li><h3>Comparison of cropped regions</h3><canvas style="margin: auto;" id="img_compare_canvas"></canvas><p>Click mouse to flip</p></li>');

  var im2_uri = [];
  im2_uri.push( "/_static/" + _vise_current_search_engine_name + "/" + im2fn );
  im2_uri.push( "?crop=false" );
  im2_uri.push( "&scale=false");
  im2_uri.push( "&draw_region=true");
  im2_uri.push( "&" + region);
  im2_uri.push( "&" + H);
  html.push( '<li><h3>Search result</h3><img src="' + im2_uri.join('') + '" /><p>' + im2fn + '</p></li>');

  content.innerHTML = html.join('');

  var crop_im1_uri = [];
  crop_im1_uri.push( "/_static/" + _vise_current_search_engine_name + "/" + im1fn );
  crop_im1_uri.push( "?crop=true" );
  crop_im1_uri.push( "&scale=false");
  crop_im1_uri.push( "&draw_region=false");
  crop_im1_uri.push( "&" + region);
  //crop_im1_uri.push( "&sw=" + _vise_query_width + "&sh=" + _vise_query_height);

  var crop_im2_uri = [];
  crop_im2_uri.push( "/_static/" + _vise_current_search_engine_name + "/" + im2fn );
  crop_im2_uri.push( "?crop=true" );
  crop_im2_uri.push( "&scale=true");
  crop_im2_uri.push( "&draw_region=false");
  crop_im2_uri.push( "&" + region);
  crop_im2_uri.push( "&" + H);
  crop_im2_uri.push( "&sw=" + _vise_query_width + "&sh=" + _vise_query_height);
  
  var imcomp_canvas = document.getElementById('img_compare_canvas');
  imcomp_canvas.addEventListener('mousedown', img_comp_canvas_mousedown_listener);
  imcomp_canvas.addEventListener('mouseup', img_comp_canvas_mouseup_listener);
  imcomp_canvas.width  = _vise_query_width;
  imcomp_canvas.height = _vise_query_height;

  _vise_load_imcomp_img( crop_im1_uri.join(''), 'im1' );
  _vise_load_imcomp_img( crop_im2_uri.join(''), 'im2' );
}

function _vise_load_imcomp_img(img_uri, name) {
  var imcomp_img = new Image();

  imcomp_img.addEventListener( "error", function() {
    _vise_show_message("Error loading image ]" + img_uri + "] !", VISE_THEME_MESSAGE_TIMEOUT_MS);
  }, false);

  imcomp_img.addEventListener( "abort", function() {
    _vise_show_message("Aborted loading image [" + img_uri + "] !", VISE_THEME_MESSAGE_TIMEOUT_MS);
  }, false);

  imcomp_img.addEventListener( "load", function() {
    switch( name ) {
      case "im1":
        imcomp_im1 = imcomp_img;
        im1_loaded = true;
        img_comp_canvas_mouseup_listener();
        break;
      case "im2":
        imcomp_im2 = imcomp_img;
        im2_loaded = true;
        break;
    } 
  });
  imcomp_img.src = img_uri;
}

function img_comp_canvas_mousedown_listener(e) {
  var imcomp_canvas = document.getElementById('img_compare_canvas');
  var imcomp_ctx = imcomp_canvas.getContext('2d');

  //imcomp_ctx.clearRect(0, 0, _vise_query_width, _vise_query_height);
  imcomp_ctx.drawImage(imcomp_im2, 0, 0);
}

function img_comp_canvas_mouseup_listener(e) {
  var imcomp_canvas = document.getElementById('img_compare_canvas');
  var imcomp_ctx = imcomp_canvas.getContext('2d');

  //imcomp_ctx.clearRect(0, 0, _vise_query_width, _vise_query_height);
  imcomp_ctx.drawImage(imcomp_im1, 0, 0);
}

//
// infinite scroll pane for image list
//

//
// helper functions
//
function _vise_toggle_advanced_settings() {
  var p = document.getElementById("vise_advanced_settings");
  if ( p.style.display == "block" ) {
    p.style.display = "none";
  } else {
    p.style.display = "block";
  }
}

//
// dired : folder selector
//
var _dired_current_path_name = "";
var _dired_current_path_folders = [];

function dired(folder) {
  fetch_dired_contents( _dired_current_path_name, folder );
}

function fetch_dired_contents( path, folder ) {
  _vise_server.open("GET", VISE_SERVER_ADDRESS + "_dired?path=" + path + "&folder=" + folder);
  _vise_server.send();
}

function _vise_handle_dired_response(json) {
  _dired_current_path_name = json.path;
  document.getElementById("dired_current_path").value = _dired_current_path_name;
  var p = document.getElementById("dired_current_path_folders");
  p.innerHTML = "";
  for ( var i=0; i < json.folders.length; i++ ) {
    var folder = document.createElement("li");
    folder.appendChild( document.createTextNode(json.folders[i]) );
    folder.setAttribute( "onclick", "dired(\"" + json.folders[i] + "\")" );
    p.appendChild( folder );
  }

}
