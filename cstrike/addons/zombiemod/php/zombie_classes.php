<html>
<head>
<LINK href="zombie_classes.css" type="text/css" rel="StyleSheet" />
<title>ZombieMod 2.0 Player Information</title>
</head>
<?php
	parse_str( $_POST["vars"] );
?>

<body bgcolor="black">
<font face="verdana" color="white" size="2"><u><b><i><img src="zmboldbloodcz1.jpg" border="0"></i></b></u></font><br />
<font color="white" size="3"><u><b><i>Zombie Class List</i></b></u></font><br />
<table width="400" height="100%">
<tr><td valign="top">
<font color="white" size="2">
<?php

$aname = $name;
$amodel = $model;
$ahealth = $health;
$aspeed = $speed;
$ajumph = $jumph;
$ahss = $hss;
$akb = $kb;
$ahso = $hso;
$aregh = $regh;
$aregs  = $regs;
$agrenm = $grenm;
$agrenk = $grenk;
$ahb = $hb;

for ( $x = 0; $x < count($aname); $x++ )
{
	$sModel = htmlspecialchars( $amodel[$x], ENT_QUOTES );
	$saModel = explode( "/", $sModel );
	$sModel = $saModel[ count($saModel) - 1 ];
	print "<p class=\"newsblock\">\r\n";
	print "	<font color=\"white\" size=\"3\"><u> " . ($x+1) . ". " . htmlspecialchars( $aname[$x], ENT_QUOTES ) . "</u></font><BR />\r\n";
	print "	<b>Model:</b> " . $sModel . "<BR />\r\n";
	print "	<b>Health:</b> " . htmlspecialchars( $ahealth[$x], ENT_QUOTES ) . "<BR />\r\n";
	print "	<b>Speed:</b> " . htmlspecialchars( $aspeed[$x], ENT_QUOTES ) . "<BR />\r\n";
	print "	<b>Jump Height:</b> " . htmlspecialchars( $ajumph[$x], ENT_QUOTES ) . "<BR />\r\n";
	print "	<b>Head Shots Req:</b> " . htmlspecialchars( $ahss[$x], ENT_QUOTES ) . "<BR />\r\n";
	print "	<b>KnockBack:</b> " . htmlspecialchars( $akb[$x], ENT_QUOTES ) . "<BR />\r\n";
	print "	<b>Head Shots Only:</b> " . (htmlspecialchars( $ahso[$x], ENT_QUOTES ) == "1" ? "Yes" : "No" ) . "<BR />\r\n";
	print "	<b>Regen Health:</b> " . htmlspecialchars( $aregh[$x], ENT_QUOTES ) . "<BR />\r\n";
	print "	<b>Regen Seconds:</b> " . htmlspecialchars( $aregs[$x], ENT_QUOTES ) . "<BR />\r\n";
	print "	<b>Grenade Multiplier:</b> " . htmlspecialchars( $agrenm[$x], ENT_QUOTES ) . "<BR />\r\n";
	print "	<b>Grenade Knockback Multiplier:</b> " . htmlspecialchars( $agrenk[$x], ENT_QUOTES ) . "<BR />\r\n";
	print "	<b>Health Bonus:</b> " . htmlspecialchars( $ahb[$x], ENT_QUOTES ) . "<BR />\r\n";
	print "</p>\r\n";
}

?>
</font></tr></td></table>

<br /><font color="white" size="1">ZombieMod v<?php print htmlspecialchars( $ver ); ?><br /><a href="http://www.zombiemod.com">www.ZombieMod.com</a><br />Copyright &copy; <?php print date('Y'); ?>. All Rights Reserved<br />Jason "c0ldfyr3" Croghan all the way from Dublin, Ireland</font></font>
</body>
</html>