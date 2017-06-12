var VISE_THEME_MESSAGE_TIMEOUT_MS = 4000;

var _vise_message_clear_timer;

// XHR
var _vise_server_get = new XMLHttpRequest();
var _vise_server_post = new XMLHttpRequest();
var _vise_messenger = new XMLHttpRequest();
var _vise_state = new XMLHttpRequest();

var VISE_SERVER_ADDRESS    = "http://localhost:9971/";
var VISE_MESSENGER_ADDRESS = VISE_SERVER_ADDRESS + "_message";
var VISE_QUERY_ADDRESS     = VISE_SERVER_ADDRESS + "_query";

_vise_server_get.addEventListener("load", _vise_server_get_response_listener);
_vise_server_post.addEventListener("load", _vise_server_post_response_listener);
_vise_messenger.addEventListener("load", _vise_message_listener);
_vise_state.addEventListener("load", _vise_state_listener);

var _vise_current_state_id = -1;
var _vise_current_state_name = "";
var _vise_current_search_engine_name = "";
var _vise_search_engine_state;

// progress tracking
var _vise_progress = {};

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
  _vise_server_send_get_request("_vise_home.html");

  // create the seed connection to receive messages
  _vise_messenger.open("GET", VISE_MESSENGER_ADDRESS);
  _vise_messenger.send();
}


//
// Vise Server ( localhost:9973 )
//
function _vise_server_get_response_listener() {
  var response_str = this.responseText;
  var content_type = this.getResponseHeader('Content-Type');

  if ( content_type.includes("text/html") ) {
    document.getElementById("content").innerHTML = response_str;
  } else {
    console.log("Received response of unknown content-type : " + content_type);
  }
}

function _vise_server_post_response_listener() {
  var response_str = this.responseText;
  var content_type = this.getResponseHeader('Content-Type');

  if ( content_type.includes("application/json") ) {
    // @todo handle post response, current assumption is that it will be OK
  } else {
    console.log("Received response of unknown content-type : " + content_type);
  }
}


function _vise_message_listener() {
  var packet = this.responseText;

  // create a connection for next message
  _vise_messenger.open("GET", VISE_MESSENGER_ADDRESS);
  _vise_messenger.send();

  _vise_handle_message( packet );
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
    //console.log("_vise_current_state_name = " + _vise_current_state_name + " : percent = " + percent + " : " + (percent % 25));

    if ( percent != _vise_progress[_vise_current_state_name] ) {
      _vise_progress[_vise_current_state_name] = percent;
      _vise_fetch_random_image();
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
          _vise_fetch_state();
          break;
        case "show":
          document.getElementById("footer_panel").style.display = "block";
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
      _vise_server_send_get_request("_vise_home.html");
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

function _vise_load_search_engine(search_engine_name) {
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
  _vise_server_post.open("POST", VISE_SERVER_ADDRESS);
  _vise_server_post.send(post_data);
}

function _vise_server_send_state_post_request(state_name, post_data) {
  _vise_server_post.open("POST", VISE_SERVER_ADDRESS + state_name);
  _vise_server_post.send(post_data);
}

function _vise_server_send_get_request(resource_name) {
  _vise_server_get.open("GET", VISE_SERVER_ADDRESS + resource_name);
  _vise_server_get.send();
}

function _vise_fetch_random_image() {
  _vise_server_get.open("GET", VISE_SERVER_ADDRESS + "_random_image");
  _vise_server_get.send();
}

//
// State
//
function _vise_state_listener() {
  var response_str = this.responseText;
  var content_type = this.getResponseHeader('Content-Type');

  if ( content_type.includes("application/json") ) {
    var json = JSON.parse(response_str);
    _vise_update_state_info(json);
  } else {
    console.log("Received response of unknown content-type : " + content_type);
  }
}

function _vise_fetch_state() {
  _vise_state.open("GET", VISE_SERVER_ADDRESS + "_state");
  _vise_state.send();
}

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
    _vise_progress[_vise_current_state_name] = 0;

    // request content for this state
    _vise_server_send_get_request( _vise_current_state_name );

    // reset the progress bar
    _vise_reset_progress_bar();

    if ( _vise_current_state_id > 2 ) {
      _vise_fetch_random_image();
    }

    // update browser title
    document.title = _vise_current_search_engine_name + " : VISE";
  }
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

