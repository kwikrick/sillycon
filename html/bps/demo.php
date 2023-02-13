<?php 
# debugging only
ini_set("display_errors", "1");
ini_set("display_startup_errors", "1");
error_reporting(E_ALL);

# input variable is $exression
@$expression = $_GET["expression"];
# set default value
if (!isset($expression)) $expression="=x2";
# set result
# TODO: don't use shell echo, but do write to file here, 
# to prevent interpretation of operators by shell
$tmpfilename = tempnam("/tmp","bps_parser");
file_put_contents($tmpfilename, $expression); 
$result = `/usr/local/bin/parser $tmpfilename`;

# ------- the page layout below is always the same, just some variables filled in -------
?>

<!DOCTYPE html PUBLIC "-//W3C//DTD HTML 4.01//EN">
<html>
	<head>
		<title>SillyCon - a silly constraint language</title>
		<link rel="stylesheet" href="../kwikrick.css">	    
	</head>
	<body>
		<div class="envelope">
			<center>
				<h1>SillyCon</h1>
				<p>A silly constraint language</p>
			</center>

			<p>This is rather silly language for expressing
			numerical constraint problems. 
			The language uses a prefix notation for operators (e.g. +4 5 means
			4+5, =x3 means x=3). 
			All operators and variables are single characters. All puctuation characters are 				operators. All alphabetic characters are variables. All sequences of
			digits are numbers. Whitespace is ignored (but may be used to separate numbers). 				Variables are 9 bit signed integers (i.e. from -256 to +255). </p>

			<p>For more details: see <a href="manual.html">The SillyCon Manual</a>.</p>

			<p> The language is very compact, but not very practical, 
			and solving problems is not fast 
			at all. The point of this language is to be esoteric and silly, 
			and to show the viability of the 
			<a href="bps.html">Boolean Propagation Solver</a> as a basis for  
			constraint based computing.</p>
            <p>
            The SillyCon interpretor is implemented in C and is available for 
<a href="source.tgz">download</a>.
            </p>
 
			<p> Try it out: Enter a numerical expression or equation <i>in prefix format</i> below. 
			Here are some examples:
				<ul>
					<li><a href="index.php?expression=%2B4+5">+4 5</a> (4+5)
					<li><a href="index.php?expression=%3Dx1">=x1</a> (x=1)
					<li><a href="index.php?expression=%3D10%2Bx1">=10+x1</a> (x+1 = 10)
					<li><a href="index.php?expression=|%3Dx1%3Dx-1">|=x1=x-1</a> (x = 1 | x = -1)
					<li><a href="index.php?expression=%26>x10<x20">&&gt;x10&lt;x20</a> ( x &gt; 10 & x &lt; 20)
					<li><a href="index.php?expression=%3D10*xy">=10*xy</a> (x*y = 10; takes a while to solve)
					<li><a href="index.php?expression=%26<z*%2B1x%2B1x%26>%2B1z*xx%26>x-1%3Dz200">x=approximate square root of z=200</a>
					<li><a href="index.php?expression=`%26%3DA1%3Dy%2B1x%26%3Dy%2B1x%26>x64<x90">A=1, B=2, ..., Z=26</a>
				</ul>	 
			</p>

			<FORM action="index.php" method="get">
			    <P>
			    <LABEL for="Expression:">Expression: </LABEL></BR>
			    <TEXTAREA row=3 columns=40 name="expression"><?php echo $expression ?></TEXTAREA>
			    <INPUT type="submit" value="Solve"> 
			    </P>
			</FORM>

			<p> Result: <pre><?php echo $result; ?></pre></p>
		</div>
	</body>
</html>
