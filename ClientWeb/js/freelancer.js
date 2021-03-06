/// @file freelancer.js
/// @author justine sabbatier
/// @version 1.5
/// @date 17/10/2014

////////////////////// VARIABLES GLOBALES

/// @def x		abscisse
var x;
/// @def y		ordonnées
var y;

var xr;
/// @def y		ordonnées
var yr;
/// @def robx	abscisse du robot
var robx;
/// @def roby		ordonnées du robot
var roby;
/// @def tobo 		orientation du robot
var tobo;
/// @def x		abscisse	
var xdot;
/// @def y		ordonnées
var ydot;
/// @def canvas		canvas du site affichant les obstacles
var canvas;
/// @def context		context du canvas
var context;
/// @def TO_RADIANS		valeur du radian
var TO_RADIANS = Math.PI/180;
/// @def robot		image du robot
var robot = new Image();
    robot.src = 'img/robotCanvas.png'; 
/// @def xmin valeur min des abscisses;
var xmin;
/// @def ymin valeur min des abscisses;
var ymin;

var xmax;

var ymax;

var xtemp;
var ytemp;

var xfinal;
var yfinal;	

var yfrob;
var xfrob;
 
 
 
////////////////////// FONCTIONS

/// @brief fonction qui envoie les requêtes 
$(document).keydown(function(e) 
{
    switch(e.which)
	{
        case 37: // left
			var command = "/left/1";
        break;

        case 38: // up
			var command = "/fwd/1";
        break;

        case 39: // right
			var command = "/right/1";
        break;

        case 40: // down
			var command = "/bwd/1";
        break;

        default: return; // exit this handler for other keys
	}
		e.preventDefault(); // prevent the default action (scroll / move caret)
		
		
	$.ajax({
       url :$('#robip').val()+command, // La ressource ciblée
       type : 'GET', // Le type de la requête HTTP.
	   statusCode : {
		   404 : function (){alert( "Robot offline")}
	   }
    });
});
///////////////////////////////////////////////////////////////////////////ACTION ROBOT

$("#start").click(function (e)
{
	e.preventDefault();
	var command="/start/1";
	$.ajax({
       url :$('#robip').val()+command, // La ressource ciblée
       type : 'GET', // Le type de la requête HTTP.
	   statusCode : {
		   404 : function (){alert( "Robot offline")}
	   }
    });
});

$("#stop").click(function (e)
{
	e.preventDefault();
	var command="/stop/1";
	$.ajax({
       url :$('#robip').val()+command, // La ressource ciblée
       type : 'GET', // Le type de la requête HTTP.
	   statusCode : {
		   404 : function (){alert( "Robot offline")}
	   }
    });
});

/// @brief fonction qui recuperer le canvas et le contexte et appelle les fonctions de communications
window.onload = function()
{	
     canvas = document.getElementById('myCanvas');
     if(!canvas)
        {
            alert("Impossible de récupérer le canvas");
            return;
        }
     context = canvas.getContext('2d');
     if(!context)
        {
            alert("Impossible de récupérer le context du canvas");
            return;
        }
		initialize();
		context.beginPath();//On démarre un nouveau tracé
		context.scale( 0.7, 0.7 );
		
		comServer(JsonCoord);
}


/// @fn function comServer(method)
/// @brief fonction qui envoie un get au serveur
/// @param method
function comServer(method)
{
	  $.ajax({
       url :$('#adress').val(), // La ressource ciblée
       type : 'GET', // Le type de la requête HTTP.
	   statusCode : {
		   404 : function (){alert( "Server offline")}
	   }
    }).done(function( msg ){method(msg);});
}

   
/// @fn function JsonCoord( monJSON)
/// @brief fonction qui évalue le json et recupere dans des variables les coordonées
/// @param monJSON		
function JsonCoord( monJSON)
{
	/*interpretation du json*/
	//monJSON.replace(/"/g,"");
	monJSON = eval(monJSON);
	/* parcours de l'array des coordonnées des obstacles*/
	for (var k in monJSON[1])
	{
		x=(monJSON[1][k][0][0]);
		y=(monJSON[1][k][0][1]);
		context.moveTo(x, y);
		x=(monJSON[1][k][1][0]);
		y=(monJSON[1][k][1][1]);
		context.lineTo(x, y);
	}	
	context.stroke();//On trace seulement les lignes.
	
	/*On cherche les mins et les max*/
	xmin=(monJSON[2][0][0]);
	ymin=(monJSON[2][0][1]);
	xmax=(monJSON[2][0][0]);
	ymax=(monJSON[2][0][1]);
	for (var l in monJSON[2])
	{		
		xtemp=(monJSON[2][l][0]);
		if( xtemp<xmin )
		{
			xmin=xtemp;
		}
		if( xmax<xtemp )
		{
			xmax=xtemp;
		}
		
		ytemp=(monJSON[2][l][1]);
		if( ytemp<ymin )
		{
			ymin=ytemp;
		}
		if( ymax<ytemp )
		{
			ymax=ytemp;
		}		
	}	
		
	for (var i in monJSON[2])
	{	
		context.beginPath();	
		xdot=(monJSON[2][i][0]);
		ydot=(monJSON[2][i][1]);
		scale(xdot, ydot);
		drawDot(xfinal, yfinal);
		context.fill();
		context.closePath();
	}

	/* coordonnées robots*/
	robo=(monJSON[0][2]);
	robx=(monJSON[0][0]);
	roby=(monJSON[0][1]);
	scaleRob(robx, roby);
	drawRotatedImage(robot, xfrob, yfrob, robo);
	/*dessins*/
	context.closePath();
} 


/// @fn function drawRotatedImage(image, x, y, angle)
/// @brief fonction qui effectue la rotation de l'image du robot en fonction de ses coordonnées
/// @param image ; variable contenant l'image
/// @param x ; variable des abscises
/// @param y ; variables des ordonnées
/// @param angle ; variable de l'angle d'orientation
function drawRotatedImage(image, x, y, angle) { 
	// save the current co-ordinate system 
	// before we screw with it
	context.save(); 
	// move to the middle of where we want to draw our image
	context.translate(x, y);
	// rotate around that point, converting our 
	// angle from degrees to radians 
	context.rotate(angle * TO_RADIANS);
	// draw it up and to the left by half the width
	// and height of the image 
	context.drawImage(image, -(image.width/2), -(image.height/2)); 
	// and restore the co-ords to how they were when we began
	context.restore(); 
}

/// @fn function drawDot( x, y)
/// @brief fonction qui dessine les points
/// @param x ; variable des abscises
/// @param y ; variables des ordonnées
function drawDot( x, y){
	context.fillStyle="#2c3e50"
	context.arc(x,y,5,0,2*Math.PI);
}


function scale(xs,ys){
		
	xfinal= (xs+Math.abs(xmin))*((window.innerWidth)/(xmax+Math.abs(xmin)));
	yfinal= (ys+Math.abs(ymin))*((window.innerHeight)/(ymax+Math.abs(ymin)));	
}

function scaleRob(xr,yr){
		
	xfrob= (xr+Math.abs(xmin))*((window.innerWidth)/(xmax+Math.abs(xmin)));
	yfrob= (yr+Math.abs(ymin))*((window.innerHeight)/(ymax+Math.abs(ymin)));	
}


/// @fn function initialize()
/// @brief fonction qui initialise la taille du canvas en fonction de la fenetre
function initialize() 
{
	// Register an event listener to
	// call the resizeCanvas() function each time 
	// the window is resized.
	window.addEventListener('resize', resizeCanvas, false);
	// Draw canvas border for the first time.
	resizeCanvas();
}


/// @fn function redraw()
/// @brief fonction qui redessine les traits en couleur #2c3e50 et en épaisseur de trait 5
function redraw() {
	context.strokeStyle = '#2c3e50';
	context.lineWidth = '5';
		}

/// @fn function redraw()
/// @brief Draw canvas border for the first time.
function resizeCanvas() 
{
	canvas.width = window.innerWidth;
	canvas.height = window.innerHeight;
	redraw();
}



////////////////////// TELECHARGER IMAGE


/**
 * This is the function that will take care of image extracting and
 * setting proper filename for the download.
 * IMPORTANT: Call it from within a onclick event.
*/
function downloadCanvas(link, canvasId, filename) {
    link.href = document.getElementById(canvasId).toDataURL();
    link.download = filename;
}

/** 
 * The event handler for the link's onclick event. We give THIS as a
 * parameter (=the link element), ID of the canvas and a filename.
*/
document.getElementById('download').addEventListener('click', function() {
    downloadCanvas(this, 'myCanvas', 'carte.png');
}, false)


////////////////////// ACTION RAFRAICHISSEMENT

$("#connect").click(function (e)
{
	e.preventDefault();
	context.clearRect(0, 0, context.canvas.width, context.canvas.height);
	context.beginPath();//On démarre un nouveau tracé
	comServer(JsonCoord);
});

$("#vider").click(function (e)
{
	e.preventDefault();
	$.get( 'http://172.31.1.123:80/resetMap.php' ); {
 	 alert( "Table vidée." );
}});


////////////////////// EFFETS SITE

// jQuery for page scrolling feature - requires jQuery Easing plugin
$(function() {
    $('.page-scroll a').bind('click', function(event) {
        var $anchor = $(this);
        $('html, body').stop().animate({
            scrollTop: $($anchor.attr('href')).offset().top
        }, 1500, 'easeInOutExpo');
        event.preventDefault();
    });
});

// Floating label headings for the contact form
$(function() {
    $("body").on("input propertychange", ".floating-label-form-group", function(e) {
        $(this).toggleClass("floating-label-form-group-with-value", !! $(e.target).val());
    }).on("focus", ".floating-label-form-group", function() {
        $(this).addClass("floating-label-form-group-with-focus");
    }).on("blur", ".floating-label-form-group", function() {
        $(this).removeClass("floating-label-form-group-with-focus");
    });
});

// Highlight the top nav as scrolling occurs
$('body').scrollspy({
    target: '.navbar-fixed-top'
})

// Closes the Responsive Menu on Menu Item Click
$('.navbar-collapse ul li a').click(function() {
    $('.navbar-toggle:visible').click();
});