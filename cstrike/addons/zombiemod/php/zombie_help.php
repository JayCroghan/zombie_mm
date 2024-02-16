<html>
<head>
<title>ZombieMod 2.0 Player Information</title>
</head>
<?php

	$count =  htmlspecialchars( $_GET['cnt'], ENT_QUOTES );
	$tele = htmlspecialchars( $_GET['tele'], ENT_QUOTES );
	$stuck = htmlspecialchars( $_GET['stuck'], ENT_QUOTES );
	$spawn = htmlspecialchars( $_GET['spawn'], ENT_QUOTES );
	$spawnas = htmlspecialchars( $_GET['spawnas'], ENT_QUOTES );
	$spawntimer = htmlspecialchars( $_GET['stimer'], ENT_QUOTES );
	$jetpack = htmlspecialchars( $_GET['jetpack'], ENT_QUOTES );
	$weapons = htmlspecialchars( $_GET['wns'], ENT_QUOTES );
	$classes = htmlspecialchars( $_GET['css'], ENT_QUOTES );
	$regen = htmlspecialchars( $_GET['rgn'], ENT_QUOTES );
	$hp = htmlspecialchars( $_GET['hp'], ENT_QUOTES );
	$ver = htmlspecialchars( $_GET['v'], ENT_QUOTES );
	$prim = htmlspecialchars( $_GET['prim'], ENT_QUOTES );
	$sec = htmlspecialchars( $_GET['sec'], ENT_QUOTES );
	$gren = htmlspecialchars( $_GET['gren'], ENT_QUOTES );

	$aWeaps = explode( "|", $weapons );

	$spawnas = ( $spawnas == 1 ? "zombie" : "human" );
?>

<body bgcolor="black"><font face="verdana" color="white" size="2"><font color="red" size="3"><u><b><i><img src="zmboldbloodcz1.jpg" border="0"></i></b></u></font><br />
<i>Please remember we are a non-profit group and provide this plugin for free.<br />
All we ask is that you show your appreciation and donate so that you can help to keep this plugin alive.<br />
You can donate by clicking <a href="http://www.zombiemod.com/donate.php">here</a></i>.<br />
<br /><b><u>For Beginners</u></b><br />
You have just joined a ZombieMod server. When the round starts <font color="yellow"><?php print $count ?></font> zombies will spawn randomly around the map.<br />
Your job is to stay alive as long as possible by fraggin those pesky things.<br/>
If you have the misfortune of being turned into a zombie, your new goal is to infect as many humans as possible!<br />
The round ends when either the humans eradicate all the zombies, or the zombies manage to infect the entire human race.<br />
<br /><b><u>Chat Commands</u></b><br />
<font color="yellow"><b>!zhelp</b></font> to view this screen.<br />

<?php
	if ( $tele > 0 )
	{
		print '<font color="yellow"><b>!ztele</b></font> to be teleported to spawn. (Can only be used <font color="yellow">' . $tele . '</font> times per round)<br />';
	}
	if ( $stuck == 1 )
	{
		print '<font color="yellow"><b>!zstuck</b></font> to try and un-stick yourself.<br />';
	}
	if ( $spawn == 1 )
	{
		print '<font color="yellow"><b>!zspawn</b></font> to respawn when dead.<br />';
	}
?>
<font color="yellow"><b>!zmenu</b></font> to view the zombie class menu.<br />

<?php
	if ( $jetpack == 1 )
	{
?>
		<br /><b><u>Jetpack</u></b><br />
		Open your console by pressing the ~ key and type the following...<br />
		<font color="yellow"><i>bind [key] +jetpack</i></font> and hit enter.<br />
		Replace [key] with the key you want to use the jetpack.<br />
<?php
	}
?>

<br /><b><u>Zombie Scream</u></b><br />
Open your console by pressing the ~ key and type the following...<br />
<font color="yellow"><i>bind [key] scream</i></font> and hit enter.<br />
Replace [key] with the key you want to make a scream when you're dead.<br />

<br /><b><u>Zombie Vision</u></b><br />
Open your console by pressing the ~ key and type the following...<br />
<font color="yellow"><i>bind [key] zombie_vision</i></font> and hit enter.<br />
Replace [key] with the key you want to enable/disable your Zombie Vision.<br />


<?php
	if ( $spawn == 1 )
	{
?>
		<br /><b><u>Respawn</u></b><br />
		Repsawning is enabled on this server.<br />
		You can respawn when dead by typing <font color="yellow">!zspawn</font>.<br />
		You will respawn in as a <font color="yellow"><?php print $spawnas; ?></font> and be given a <font color="yellow"><?print $prim; ?></font> and a <font color="yellow"><?print $sec; ?></font> and <font color="yellow"><?print $gren; ?></font> grenades.<br />
<?php
	}
	if ( $spawntimer > 0 )
	{
		print 'Or you can alternatively wait for the <font color="yellow">' . $spawntimer . '</font> second respawn timer.<br />';
	}
?>

<?php
	if ( strlen( $weapons ) != 0 )
	{
?>
	<br /><b><u>Restrictions</u></b><br />
	The following is the list of weapons currently restricted on this server.<br />
<?php
		for ( $x = 0; $x < (count( $aWeaps ) - 1); $x++ )
		{
			print '<font color="yellow">» ' . LookupWeaponName( $aWeaps[$x] ) . '</i></font>.<br />';
		}
	}
?>

<?php
	if ( $classes == 1 )
	{
?>
		<br /><b><u>Zombie Classes</u></b><br />
		There are a number of different classes available to you as a player. To see the list type <font color="yellow">!zmenu</font> in chat.<br />
		Each class comes with a different variation of attributes for <i>speed, knockback, health and jump height</i>.<br />
<?php
	}
?>

<?php
	if ( $regen > 0 )
	{
?>
		<br /><b><u>Health Regeneration</u></b><br />
		Health Regeneration is enabled on this server. You will regenerate <font color="yellow"><?php print $hp; ?></font> hp every <font color="yellow"><?php print $regen; ?></font> second(s).<br />
		<br />
<?php
	}
?>

<br /><font size="1">ZombieMod v<?php print $ver; ?><br /><a href="http://www.zombiemod.com">www.ZombieMod.com</a><br />Copyright &copy; <?php print date('Y'); ?>. All Rights Reserved<br />Jason "c0ldfyr3" Croghan all the way from Dublin, Ireland</font></font>
</body>
</html>

<?php
	function LookupWeaponName( $index )
	{
		$ID["CSW_NONE"] = 0;
		$ID["CSW_P228"] = 1;
		$ID["CSW_GLOCK"] = 2;
		$ID["CSW_SCOUT"] = 3;
		$ID["CSW_HEGRENADE"] = 4;
		$ID["CSW_XM1014"] = 5;
		$ID["CSW_C4"] = 6;
		$ID["CSW_MAC10"] = 7;
		$ID["CSW_AUG"] = 8;
		$ID["CSW_SMOKEGRENADE"] = 9;
		$ID["CSW_ELITE"] = 10;
		$ID["CSW_FIVESEVEN"] = 11;
		$ID["CSW_UMP45"] = 12;
		$ID["CSW_SG550"] = 13;
		$ID["CSW_GALIL"] = 14;
		$ID["CSW_FAMAS"] = 15;
		$ID["CSW_USP"] = 16;
		$ID["CSW_AWP"] = 17;
		$ID["CSW_MP5"] = 18;
		$ID["CSW_M249"] = 19;
		$ID["CSW_M3"] = 20;
		$ID["CSW_M4A1"] = 21;
		$ID["CSW_TMP"] = 22;
		$ID["CSW_G3SG1"] = 23;
		$ID["CSW_FLASHBANG"] = 24;
		$ID["CSW_DEAGLE"] = 25;
		$ID["CSW_SG552"] = 26;
		$ID["CSW_AK47"] = 27;
		$ID["CSW_KNIFE"] = 28;
		$ID["CSW_P90"] = 29;
		$ID["CSW_UNUSED1"] = 30;
		$ID["CSW_UNUSED2"] = 31;
		$ID["CSW_UNUSED3"] = 32;
		$ID["CSW_UNUSED4"] = 33;
		$ID["CSW_PRIMAMMO"] = 34;
		$ID["CSW_SECAMMO"] = 35;
		$ID["CSW_NVGS"] = 36;
		$ID["CSW_VEST"] = 37;
		$ID["CSW_VESTHELM"] = 38;
		$ID["CSW_DEFUSEKIT"] = 39;

		switch ($index)
		{
			case $ID["CSW_P228"]:
				return "228 Compact";
			case $ID["CSW_GLOCK"]:
				return "9x19mm Sidearm";
			case $ID["CSW_SCOUT"]:
				return "Schmidt Scout";
			case $ID["CSW_HEGRENADE"]:
				return "HE Grenade";
			case $ID["CSW_XM1014"]:
				return "Leone YG1265 Auto Shotgun";
			case $ID["CSW_C4"]:
				return "C4 Bomb";
			case $ID["CSW_MAC10"]:
				return "Ingram MAC-10";
			case $ID["CSW_AUG"]:
				return "Bullpup";
			case $ID["CSW_SMOKEGRENADE"]:
				return "Smoke Grenade";
			case $ID["CSW_ELITE"]:
				return ".40 Dual Elites";
			case $ID["CSW_FIVESEVEN"]:
				return "ES Five//Seven";
			case $ID["CSW_UMP45"]:
				return "KM UMP45";
			case $ID["CSW_SG550"]:
				return "Krieg 550 Commando";
			case $ID["CSW_GALIL"]:
				return "IDF Defender";
			case $ID["CSW_FAMAS"]:
				return "Clarion 5.56";
			case $ID["CSW_USP"]:
				return "KM .45 Tactical";
			case $ID["CSW_AWP"]:
				return "Magnum Sniper Rifle";
			case $ID["CSW_MP5"]:
				return "KM Sub-Machine Gun";
			case $ID["CSW_M249"]:
				return "M249";
			case $ID["CSW_M3"]:
				return "Leone 12 Gauge Super";
			case $ID["CSW_M4A1"]:
				return "Maverick M4A1 Carbine";
			case $ID["CSW_TMP"]:
				return "Schmidt Machine Pistol";
			case $ID["CSW_G3SG1"]:
				return "D3/AU1";
			case $ID["CSW_FLASHBANG"]:
				return "Flashbang";
			case $ID["CSW_DEAGLE"]:
				return "Night Hawk .50C";
			case $ID["CSW_SG552"]:
				return "Krieg 552";
			case $ID["CSW_AK47"]:
				return "CV-47";
			case $ID["CSW_KNIFE"]:
				return "Knife";
			case $ID["CSW_P90"]:
				return "ES C90";
			case $ID["CSW_PRIMAMMO"]:
				return "Primary Ammo";
			case $ID["CSW_SECAMMO"]:
				return "Secondary Ammo";
			case $ID["CSW_NVGS"]:
				return "Nightvision Goggles";
			case $ID["CSW_VEST"]:
				return "Vest";
			case $ID["CSW_VESTHELM"]:
				return "Vest & Helmet";
			case $ID["CSW_DEFUSEKIT"]:
				return "Defusal Kit";
			default:
				return "Unknown";
		}
	}
?>