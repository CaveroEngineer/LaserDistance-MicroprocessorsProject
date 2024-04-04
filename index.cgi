t <!DOCTYPE html>
t     <html>
t       <head>
t         <meta content="text/html; charset=UTF-8" http-equiv="content-type">
t         <meta http-equiv="refresh" content="5; url=http://192.168.1.189/index.cgi">
t         <title>Proyecto Final Ruben Cavero y Andres</title>
t       </head>
t       <body> 
t         <form id="formulario" action="/index.cgi" method="get"> 
t           <table style="width: 200px" border="0">
t             <tbody>
t               <tr align="center">
t                 <td border="1" style="width: 100%">
t                   <div style="font-weight:bold">
t                     Proyecto Final Ruben Cavero y Andres
t                   </div>
t                 </td>
t               </tr>
t               <tr align="center">
t                 <td border="1" style="width: 100%">
t                   <div style="font-weight:bold">
t                     <button name="ModoManual" value="ON"  type="submit">Modo Manual</button>
t                   </div>
t                 </td>
t               </tr>
t               <tr align="center">
t                 <td border="1" style="width: 100%">
t                   <div style="font-weight:bold">
t                     <button name="ModoAutomatico" value="ON"  type="submit">Modo Automatico</button>
t                   </div>
t                 </td>
t               </tr>
t               <tr align="center">
t                 <td border="1" style="width: 100%">
t                   <div style="font-weight:bold">
t                     <button name="ModoConfiguracion" value="ON"  type="submit">Modo Configuracion</button>
t                   </div>
t                 </td>
t               </tr>
t               <tr align="center">
t                 <td border="1" style="width: 100%">
t                   <div style="font-weight:bold">
t                            Alarma:
t                   </div>
t                 </td>
t               </tr>
t               <tr align="center">
t                 <td border="1" style="width: 100%">
t                   <div style="font-weight:bold">
t                     <button name="ApagarAlarma" value="ON"  type="submit">Apagar Alarma</button>
t                   </div>
t                 </td>
t               </tr>
t               <tr align="center">
t                 <td border="1" style="width: 100%">
t                   <div style="font-weight:bold">
t                     <button name="EncenderAlarma" value="ON"  type="submit">Encender Alarma</button>
t                   </div>
t                 </td>
t               </tr>
t             </tbody>
t           </table>
t           <table style="width: 200px" border="0">
t             <tbody>
t               <tr align="center">
c a 3             <td border="1" bgcolor="%s" style="width: 45%%">
t                   <div style="color:white;font-weight:bold">Alarma Desactivada</div>
t                 </td>
t                 <td style="width: 10%">    <br>
t                 </td>
c a 4             <td border="1" bgcolor="%s"  style="width: 45%%">
t                   <div style="color:white;font-weight:bold">Alarma Activada</div>
t                 </td>
t               </tr>
t               <tr align="center">
t                 <td border="1" style="width: 100%">
t                   <div style="font-weight:bold">
t                            Mediciones:
t                   </div>
t                 </td>
t               </tr align="center">
c a 5                <td align="left"><strong style="font-size: 17px;color: #2b301;">Angulo del Motor: %d &#x2103</strong></td>
t               </tr>
t               </tr align="center">
c a 6                <td align="left"><strong style="font-size: 17px;color: #2b301;">Temperatura: %0.1f &#x2103</strong></td>
t               </tr>
t               </tr align="center">
c a 7                <td align="left"><strong style="font-size: 17px;color: #2b301;">Distancia: %0.1f &#x2103</strong></td>
t               </tr>
t               </tr align="center">
c a 8                <td align="left"><strong style="font-size: 17px;color: #2b301;">Brillo: %d &#x2103</strong></td>
t               </tr>
t             </tbody>
t           </table>
t         </form>
t         <br>
t       </body>
t     </html>
.