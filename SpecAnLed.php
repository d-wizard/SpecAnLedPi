
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

      // Check which the button was pressed.

      // Gain / Brightness
      if(isset($_POST["E_GAIN_BRIGHT_LOCAL"]))
      {
         shell_exec($pythonSetGetScript." E_GAIN_BRIGHT_LOCAL");
         $buttonPressed = True;
      }
      if(isset($_POST["E_GAIN_BRIGHT_REMOTE"]))
      {
         shell_exec($pythonSetGetScript." E_GAIN_BRIGHT_REMOTE");
         $buttonPressed = True;
      }
      if(isset($_POST["E_GAIN_VALUE"]))
      {
         shell_exec($pythonSetGetScript." E_GAIN_VALUE".$_POST["gain_slider"]);
         $buttonPressed = True;
      }
      if(isset($_POST["E_BRIGHT_VALUE"]))
      {
         shell_exec($pythonSetGetScript." E_BRIGHT_VALUE".$_POST["bright_slider"]);
         $buttonPressed = True;
      }

      // Gradient
      if(isset($_POST["E_GRADIENT_POS"]))
      {
         shell_exec($pythonSetGetScript." E_GRADIENT_POS");
         $buttonPressed = True;
      }
      if(isset($_POST["E_GRADIENT_NEG"]))
      {
         shell_exec($pythonSetGetScript." E_GRADIENT_NEG");
         $buttonPressed = True;
      }

      // Display Type
      if(isset($_POST["E_DISPLAY_CHANGE_POS"]))
      {
         shell_exec($pythonSetGetScript." E_DISPLAY_CHANGE_POS");
         $buttonPressed = True;
      }
      if(isset($_POST["E_DISPLAY_CHANGE_NEG"]))
      {
         shell_exec($pythonSetGetScript." E_DISPLAY_CHANGE_NEG");
         $buttonPressed = True;
      }

      // Reverse Gradient Toggle
      if(isset($_POST["E_REVERSE_GRADIENT_TOGGLE"]))
      {
         shell_exec($pythonSetGetScript." E_REVERSE_GRADIENT_TOGGLE");
         $buttonPressed = True;
      }

      if($buttonPressed != False)
      {
         header("Location: #");
      }
   ?>

   <center>
      <br>
      <form action="SpecAnLed.php" method="post">
         <h1>Gain / Brightness</h1>
         <table cellpadding="5">
            <tr>
               <td style="vertical-align: middle">Gain</td>
               <td><input name="gain_slider" type="range" min="0" max="100" value="50" class="slider"></td>
               <td><center><input name="E_GAIN_VALUE" class="block" type="submit" value="Update" /></center></td>
            </tr>
            <tr>
               <td style="vertical-align: middle">Brightness</td>
               <td><input name="bright_slider" type="range" min="0" max="1.0" value="0.5" step="0.02" class="slider"></td>
               <td><center><input name="E_BRIGHT_VALUE" class="block" type="submit" value="Update" /></center></td>
            </tr>
         </table>
         <table cellpadding="5">
            <tr>
               <td><center><input name="E_GAIN_BRIGHT_LOCAL" class="block" type="submit" value="Local Control" /></center></td>
               <td><center><input name="E_GAIN_BRIGHT_REMOTE" class="block" type="submit" value="Remote Control" /></center></td>
            </tr>
         </table>
         <br><hr>
         <h1>Gradient Display</h1>
         <table cellpadding="5">
            <tr>
               <td style="vertical-align: middle">Gradient Type</td>
               <td><center><input name="E_GRADIENT_POS" class="block" type="submit" value="<" /></center></td>
               <td><center><input name="E_GRADIENT_NEG" class="block" type="submit" value=">" /></center></td>
            </tr>
            <tr>
               <td style="vertical-align: middle">Display Type</td>
               <td><center><input name="E_DISPLAY_CHANGE_POS" class="block" type="submit" value="<" /></center></td>
               <td><center><input name="E_DISPLAY_CHANGE_NEG" class="block" type="submit" value=">" /></center></td>
            </tr>
            <tr>
               <td style="vertical-align: middle">Gradient Direction</td>
               <td colspan="2"><center><input name="E_REVERSE_GRADIENT_TOGGLE" class="block" type="submit" value="Toggle" /></center></td>
            </tr>
         </table>
      </form>

   </center>
   </body>

</html>