
<html>
<head>
<link rel="stylesheet" type="text/css" href="web.css">
<meta name="viewport" content="width=device-width" />
<style>
</style>
<title>Spec An LED Control</title>
</head>
   <body style='background-color : black' >
   
   <?php
      /////////////////////////////////////////////////////////
      // This code will run every time something happens.
      /////////////////////////////////////////////////////////
      //
      $thisScriptDir = dirname(__FILE__);
      $pythonSetGetScript = "python ".$thisScriptDir."/sendCmds.py";
      $buttonPressed = False;

      $gainValue = 50;
      $brightnessValue = "0.5";

      // Check which the button was pressed.

      // Gain / Brightness
      if(isset($_GET["gain_local"]))
      {
         shell_exec($pythonSetGetScript." E_GAIN_BRIGHT_LOCAL");
         $buttonPressed = True;
      }
      if(isset($_GET["gain_remote"]))
      {
         shell_exec($pythonSetGetScript." E_GAIN_BRIGHT_REMOTE");
         $buttonPressed = True;
      }
      if(isset($_GET["gain_val"]))
      {
         shell_exec($pythonSetGetScript." E_GAIN_VALUE".$_GET["gain_slider"]);
         $buttonPressed = True;
      }
      if(isset($_GET["bright_val"]))
      {
         shell_exec($pythonSetGetScript." E_BRIGHT_VALUE".$_GET["bright_slider"]);
         $buttonPressed = True;
      }

      // Gradient
      if(isset($_GET["grad_pos"]))
      {
         shell_exec($pythonSetGetScript." E_GRADIENT_POS");
         $buttonPressed = True;
      }
      if(isset($_GET["grad_neg"]))
      {
         shell_exec($pythonSetGetScript." E_GRADIENT_NEG");
         $buttonPressed = True;
      }

      // Display Type
      if(isset($_GET["disp_pos"]))
      {
         shell_exec($pythonSetGetScript." E_DISPLAY_CHANGE_POS");
         $buttonPressed = True;
      }
      if(isset($_GET["disp_neg"]))
      {
         shell_exec($pythonSetGetScript." E_DISPLAY_CHANGE_NEG");
         $buttonPressed = True;
      }

      // Reverse Gradient Toggle
      if(isset($_GET["grad_rev"]))
      {
         shell_exec($pythonSetGetScript." E_REVERSE_GRADIENT_TOGGLE");
         $buttonPressed = True;
      }

      // Check if Gain / Brightness values are specified.
      if(isset($_GET["gain_slider"]))
      {
         $gainValue=$_GET["gain_slider"];
      }
      if(isset($_GET["bright_slider"]))
      {
         $brightnessValue=$_GET["bright_slider"];
      }

   ?>

   <center>
   <form action="<?php $_PHP_SELF ?>" method="get">
      <br>
         <h1>Gain / Brightness</h1>
         <table cellpadding="5">
            <tr>
               <td style="vertical-align: middle">Gain</td>
               <td><input name="gain_slider" type="range" min="0" max="100" value=<?php echo $gainValue;?> class="slider"></td>
               <td><input name="gain_val" class="block" type="submit" value="Update" /></td>
            </tr>
            <tr>
               <td style="vertical-align: middle">Brightness</td>
               <td><input name="bright_slider" type="range" min="0" max="1.0" value=<?php echo $brightnessValue;?> step="0.02" class="slider"></td>
               <td><input name="bright_val" class="block" type="submit" value="Update" /></td>
            </tr>
         </table>
         <table cellpadding="5">
            <tr>
               <td><input name="gain_local" class="block" type="submit" value="Local Control" /></td>
               <td><input name="gain_remote" class="block" type="submit" value="Remote Control" /></td>
            </tr>
         </table>
         <br><hr>
         <h1>Gradient Display</h1>
         <table cellpadding="5">
            <tr>
               <td style="vertical-align: middle">Gradient Type</td>
               <td><input name="grad_pos" class="block" type="submit" value="<" /></td>
               <td><input name="grad_neg" class="block" type="submit" value=">" /></td>
            </tr>
            <tr>
               <td style="vertical-align: middle">Display Type</td>
               <td><input name="disp_pos" class="block" type="submit" value="<" /></td>
               <td><input name="disp_neg" class="block" type="submit" value=">" /></td>
            </tr>
            <tr>
               <td style="vertical-align: middle">Gradient Direction</td>
               <td colspan="2"><input name="grad_rev" class="block" type="submit" value="Toggle" /></td>
            </tr>
         </table>

   </form>
   </center>
   </body>

</html>