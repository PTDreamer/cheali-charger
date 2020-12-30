<?php
// Initialize the session
session_start();

// Check if the user is logged in, if not then redirect him to login page
if(!isset($_SESSION["loggedin"]) || $_SESSION["loggedin"] !== true){
    header("location: login.php");
    exit;
}
require_once "config.php";

$COND_NiXX          = 1;
$COND_Pb            = 2;
$COND_LiXX          = 4;
$COND_NiZn          = 8;
$COND_Unknown       = 16;
$COND_LED           = 32;

$COND_enableT       = 256;
$COND_enable_dV     = 512;
$COND_enable_dT     = 1024;
$COND_advanced      = 32768;

$COND_LiXX_NiZn             = ($COND_LiXX + $COND_NiZn);
$COND_LiXX_NiZn_Pb          = ($COND_LiXX + $COND_NiZn + $COND_Pb);
$COND_LiXX_NiZn_Pb_Unkn     = ($COND_LiXX + $COND_NiZn + $COND_Pb + $COND_Unknown);
$COND_NiXX_Pb               = ($COND_NiXX + $COND_Pb);

$COND_BATTERY               = ($COND_NiXX + $COND_Pb + $COND_LiXX + $COND_NiZn + $COND_Unknown);
$COND_BATT_UNKN             = ($COND_NiXX + $COND_Pb + $COND_LiXX + $COND_NiZn);

$id = isset($_GET['id']) && is_numeric($_GET['id']) ? (int)$_GET['id'] : 1;
$sql = 'SELECT * FROM batteries WHERE idx = ?';
if($stmt = mysqli_prepare($link, $sql)){
            // Bind variables to the prepared statement as parameters
    $param1 = ($page-1)*$records_per_page;
    mysqli_stmt_bind_param($stmt, "i", $id);   
    if($stmt->execute()) {
        $stmt->bind_result($idx, $name, $type, $capacity, $cells, $Ic, $Id, $Vc_per_cell, $Vd_per_cell, $minIc, $minId, $time,$enable_externT, $externTCO, $enable_adaptiveDischarge, $DCRestTime, $capCutoff, $Vs_per_cell, $balancer_error, $delta_v_enable, $delta_v, $delta_v_ignore_t, $delta_t, $dc_cycles, $date_created);
        $stmt->fetch();
        $stmt->close();
                  //  while ($stmt->fetch()) {
                    //    printf ("%s (%s)\n", $name, $name);
                    //}
        /* close statement */
//    $stmt->close();
    }
}
// Fetch the records so we can display them in our template.
?>
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="utf-8">
    <meta http-equiv="X-UA-Compatible" content="IE=edge">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>Bootstrap User Management Data Table</title>
    <link rel="stylesheet" href="https://fonts.googleapis.com/css?family=Roboto|Varela+Round">
    <link rel="stylesheet" href="https://fonts.googleapis.com/icon?family=Material+Icons">
    <link rel="stylesheet" href="https://maxcdn.bootstrapcdn.com/font-awesome/4.7.0/css/font-awesome.min.css">
    <link rel="stylesheet" href="https://maxcdn.bootstrapcdn.com/bootstrap/4.4.1/css/bootstrap.min.css">
    <script src="https://ajax.googleapis.com/ajax/libs/jquery/3.4.1/jquery.min.js"></script>
    <script src="https://cdnjs.cloudflare.com/ajax/libs/popper.js/1.16.0/umd/popper.min.js"></script>
    <script src="https://maxcdn.bootstrapcdn.com/bootstrap/4.4.1/js/bootstrap.min.js"></script>
    <style type="text/css">
        body {
            color: #566787;
            background: #f5f5f5;
            font-family: 'Varela Round', sans-serif;
            font-size: 13px;
        }
        .table-wrapper {
            background: #fff;
            padding: 20px 25px;
            margin: 30px 0;
            border-radius: 3px;
            box-shadow: 0 1px 1px rgba(0,0,0,.05);
        }
        .table-title {
          padding-bottom: 15px;
          background: #299be4;
          color: #fff;
          padding: 16px 30px;
          margin: -20px -25px 10px;
          border-radius: 3px 3px 0 0;
      }
      .table-title h2 {
          margin: 5px 0 0;
          font-size: 24px;
      }
      .table-title .btn {
          color: #566787;
          float: right;
          font-size: 13px;
          background: #fff;
          border: none;
          min-width: 50px;
          border-radius: 2px;
          border: none;
          outline: none !important;
          margin-left: 10px;
      }
      .table-title .btn:hover, .table-title .btn:focus {
        color: #566787;
        background: #f2f2f2;
    }
    .table-title .btn i {
      float: left;
      font-size: 21px;
      margin-right: 5px;
  }
  .table-title .btn span {
      float: left;
      margin-top: 2px;
  }
  table.table tr th, table.table tr td {
    border-color: #e9e9e9;
    padding: 12px 15px;
    vertical-align: middle;
}
table.table tr th:first-child {
  width: 60px;
}
table.table tr th:last-child {
  width: 100px;
}
table.table-striped tbody tr:nth-of-type(odd) {
   background-color: #fcfcfc;
}
table.table-striped.table-hover tbody tr:hover {
  background: #f5f5f5;
}
table.table th i {
    font-size: 13px;
    margin: 0 5px;
    cursor: pointer;
}	
table.table td:last-child i {
  opacity: 0.9;
  font-size: 22px;
  margin: 0 5px;
}
table.table td a {
  font-weight: bold;
  color: #566787;
  display: inline-block;
  text-decoration: none;
}
table.table td a:hover {
  color: #2196F3;
}
table.table td a.settings {
    color: #2196F3;
}
table.table td a.delete {
    color: #F44336;
}
table.table td a.use_bat {
    color: #33cc33;
}
table.table td i {
    font-size: 19px;
}
table.table .avatar {
  border-radius: 50%;
  vertical-align: middle;
  margin-right: 10px;
}
.status {
  font-size: 30px;
  margin: 2px 2px 0 0;
  display: inline-block;
  vertical-align: middle;
  line-height: 10px;
}
.text-success {
    color: #10c469;
}
.text-info {
    color: #62c9e8;
}
.text-warning {
    color: #FFC107;
}
.text-danger {
    color: #ff5b5b;
}
.pagination {
    float: right;
    margin: 0 0 5px;
}
.pagination li a {
    border: none;
    font-size: 13px;
    min-width: 30px;
    min-height: 30px;
    color: #999;
    margin: 0 2px;
    line-height: 30px;
    border-radius: 2px !important;
    text-align: center;
    padding: 0 6px;
}
.pagination li a:hover {
    color: #666;
}	
.pagination li.active a, .pagination li.active a.page-link {
    background: #03A9F4;
}
.pagination li.active a:hover {        
    background: #0397d6;
}
.pagination li.disabled i {
    color: #ccc;
}
.pagination li i {
    font-size: 16px;
    padding-top: 6px
}
.hint-text {
    float: left;
    margin-top: 10px;
    font-size: 13px;
}
</style>
</head>
<body>
    <div class="container">
        <form>
            <div class="form-group">
             <?php 
             $imgs = glob("./images/$idx.{jpg,jpeg,png,gif}", GLOB_BRACE);
             if(empty($imgs))
                $img = "/images/no-image-available.jpg";
            else
                $img = $imgs[0];
            ?>
            <img id="bat_image" src=<?php echo $img?>> <?php $name?> 
            <div class="custom-file mb-3">
              <input type="file" class="custom-file-input" id="customFile" name="upfile">
              <label class="custom-file-label" for="customFile">Choose file</label>
          </div>
      </div>
      <div class="form-group">
        <label for="name">Name</label>
        <textarea class="form-control" id="name" rows="1" value=><?php echo $name;?></textarea>
    </div>
    <div class="form-group">
        <label for="type">Battery type</label>
        <select class="form-control" id="type">
            <?php
            $sql = 'SELECT * FROM battery_types';
            if($stmt = mysqli_prepare($link, $sql)){  
                if($stmt->execute()) {
                    $stmt->bind_result($bat_type, $bat_str);
                    while ($stmt->fetch()) {?>
                        <option <?php if($type == $bat_type) echo selected?>> <?php echo $bat_str;?></option>
                    <?php }}}; ?>
                </select>
            </div>                
            <div class="form-group">
                <label for="capacity">Capacity</label>
                <input type="number" class="form-control" id="capacity" value=<?php echo $capacity?>>
            </div>
            <div class="form-group">
                <label for="charge_cur">Charge Current</label>
                <input type="number" class="form-control" id="char_cur" value=<?php echo $Ic?>>
            </div>
            <div class="form-group">
                <label for="discharge_cur">Discharge Current</label>
                <input type="number" class="form-control" id="dischar_cur" value=<?php echo $Id?>>
            </div>
            <div class="form-group">
                <label for="cells">Number of Cells</label>
                <select class="form-control" id="exampleFormControlSelect1">
                    <option <?php if($cells == 1) echo selected?>>1</option>
                    <option <?php if($cells == 2) echo selected?>>2</option>
                    <option <?php if($cells == 3) echo selected?>>3</option>
                    <option <?php if($cells == 4) echo selected?>>4</option>
                    <option <?php if($cells == 5) echo selected?>>5</option>
                    <option <?php if($cells == 6) echo selected?>>6</option>
                </select>
            </div>
            <div class="form-group" min="0" max="5000" condition=<?php echo ($COND_advanced + $LiXX_NiZn_Pb)?>>
                <label for="Vc_per_cell">Charge voltage per cell</label>
                <input type="number" class="form-control" id="Vc_per_cell" value=<?php echo $Vc_per_cell?>>
            </div>
            <div class="form-group" min="0" max="27000" condition=<?php echo ($COND_Unknown)?>>
                <label for="Vc_per_cell">Charge voltage per cell</label>
                <input type="number" class="form-control" id="Vc_per_cell" value=<?php echo $Vc_per_cell?>>
            </div>
            <div class="form-group" min="1200" max="2000" condition=<?php echo ($COND_advanced + $COND_NiXX)?>>
                <label for="Vc_per_cell">Voltage Cut Off</label>
                <input type="number" class="form-control" id="Vc_per_cell" value=<?php echo $Vc_per_cell?>>
            </div>
            <div class="form-group" min="1" max="27000" condition=<?php echo ($COND_LED)?>>
                <label for="Vc_per_cell">Voltage Cut Off</label>
                <input type="number" class="form-control" id="Vc_per_cell" value=<?php echo $Vc_per_cell?>>
            </div>
            <div class="form-group" min="0" max="5000" condition=<?php echo ($COND_advanced + $COND_LiXX)?>>
                <label for="Vs_per_cell">Storage voltage per cell</label>
                <input type="number" class="form-control" id="Vs_per_cell" value=<?php echo $Vs_per_cell?>>
            </div>
            <div class="form-group" min="0" max="5000" condition=<?php echo ($COND_BATT_UNKN + $COND_advanced)?>>
                <label for="Vd_per_cell">Discharged voltage per cell</label>
                <input type="number" class="form-control" id="Vd_per_cell" value=<?php echo $Vd_per_cell?>>
            </div>
            <div class="form-group" min="0" max="270000" condition=<?php echo ($COND_Unknown)?>>
                <label for="Vd_per_cell">Discharged voltage per cell</label>
                <input type="number" class="form-control" id="Vd_per_cell" value=<?php echo $Vd_per_cell?>>
            </div>
            <div class="form-group" min="1000" max="65000" condition=<?php echo ($COND_BATTERY)?>>
                <label for="capacity">Capacity (mAh)</label>
                <input type="number" class="form-control" id="capacity" value=<?php echo $capacity?>>
            </div>
            <div class="form-group" min="1" max="5000" condition=<?php echo ($COND_BATTERY + $COND_LED)?>>
                <label for="Ic">Charge current (mAh)</label>
                <input type="number" class="form-control" id="Ic" value=<?php echo $Ic?>>
            </div>
            <div class="form-group" min="1" max="5000" condition=<?php echo ($COND_advanced + $COND_LiXX_NiZn_Pb_Unkn)?>>
                <label for="minIc">Minimum charge current (mAh)</label>
                <input type="number" class="form-control" id="minIc" value=<?php echo $minIc?>>
            </div>
            <div class="form-group" min="1" max="1000" condition=<?php echo ($COND_BATTERY)?>>
                <label for="Id">Discharge current (mAh)</label>
                <input type="number" class="form-control" id="Id" value=<?php echo $Id?>>
            </div>
            <div class="form-group" min="1" max="1000" condition=<?php echo ($COND_advanced + $COND_BATTERY)?>>
                <label for="minId">Minimum discharge current (mAh)</label>
                <input type="number" class="form-control" id="minId" value=<?php echo $minId?>>
            </div>
            <div class="form-group" min="3" max="200" condition=<?php echo ($COND_advanced + $COND_LiXX_NiZn)?>>
                <label for="balancer_error">Balancer error (mV)</label>
                <input type="number" class="form-control" id="balancer_error" value=<?php echo $balancer_error?>>
            </div>
            <div class="form-group" condition=<?php echo ($COND_NiXX)?>>
                <label for="delta_v_enable">Enable delta V</label>
                <input type="checkbox" class="form-check-input" id="delta_v_enable" value=<?php echo $delta_v_enable?>>
            </div>
            <div class="form-group" min="-20" max="0" condition=<?php echo ($COND_enable_dV)?>>
                <label for="delta_v">Delta V (mV)</label>
                <input type="number" class="form-control" id="delta_v" value=<?php echo $delta_v?>>
            </div>
            <div class="form-group" min="1" max="30" condition=<?php echo ($COND_enable_dV)?>>
                <label for="delta_v_ignore_t">Ignore first (m)</label>
                <input type="number" class="form-control" id="delta_v_ignore_t" value=<?php echo $delta_v_ignore_t?>>
            </div>
            <div class="form-group" condition=<?php echo ($COND_BATTERY)?>>
                <label for="enable_externT">Enable external temp sensor</label>
                <input type="checkbox" class="form-check-input" id="enable_externT" value=<?php echo $enable_externT?>>
            </div>
            <div class="form-group" min="0.1" max="9" condition=<?php echo ($COND_enable_dT)?>>
                <label for="delta_t">Maximum external temperature difference per minute (C/m)</label>
                <input type="number" class="form-control" id="delta_t" value=<?php echo $delta_t/100?>>
            </div>
            <div class="form-group" min="1" max="99" condition=<?php echo ($COND_enable_T)?>>
                <label for="externTCO">Minimum external temperature cutoff CHECK MIN MAX of all temps</label>
                <input type="number" class="form-control" id="externTCO" value=<?php echo $externTCO/100?>>
            </div>
            <button type="submit" class="btn btn-primary">Submit</button>
        </form>
    </div>  
</div>   
<script src="https://ajax.aspnetcdn.com/ajax/jquery/jquery-1.9.0.min.js"></script>
<script type="text/javascript">
        // Add the following code if you want the name of the file appear on select
        $(".custom-file-input").on("change", function() {
          var fileName = $(this).val().split("\\").pop();
          $(this).siblings(".custom-file-label").addClass("selected").html(fileName);
      });

        $( "#picture button" ).on( "click", function( event ) {

        });

        document.getElementById("customFile").addEventListener("change", function (event) {
            compress(event);
        });
        function compress(e) {
            const width = 400;
            const height = 300;
            const fileName = e.target.files[0].name;
            const reader = new FileReader();
            reader.readAsDataURL(e.target.files[0]);
            reader.onload = event => {
                const img = new Image();
                img.src = event.target.result;
                img.onload = () => {
                    var preview = document.querySelector('#bat_image');
                    const elem = document.createElement('canvas');
                    const scaleFactor = width / img.width;
                    elem.width = width;
                    elem.height = img.height * scaleFactor;
                    const ctx = elem.getContext('2d');
                // img.width and img.height will contain the original dimensions
                ctx.drawImage(img, 0, 0, width, img.height * scaleFactor);
                ctx.canvas.toBlob((blob) => {
                    preview.src = window.URL.createObjectURL(blob);
                }, 'image/jpeg', 1);
            },
            reader.onerror = error => console.log(error);
        };
    }
</script>
</body>
</html>                                		                            