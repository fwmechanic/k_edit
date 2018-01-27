### https://ss64.com/ps/syntax-comments.html  example 1
Copy-Item demo.msi C:\install\demo.msi #copy the installer

<#
   Running this script will make your computer catch fire!
   Last updated: 1666-09-02
#>

Get-Content -Path <#configuration file#> C:\app64.ini


### https://ss64.com/ps/syntax-esc.html
echo 1 `# 1
echo 1 # 1


$msg = 'Every "lecture" should cost $5000'

$msg = "Every ""lecture"" should cost `$5000"
$msg = "Every 'lecture' should cost `$5000"
$var = 45
"The value of " + '$var' + "is '$var'"
"The value of `$var is '$var'"
$query = "SELECT * FROM Customers WHERE Name LIKE '%JONES%'"


$myHereString = @'
some text with "quotes" and variable names $printthis
some more text
'@

$anotherHereString = @"
The value of `$var is $var
some more text
"@
