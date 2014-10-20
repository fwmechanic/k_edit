require"json"

local function foreach(tab, func) for k, v in pairs(tab) do func(k,v) end end

local function printValue( name, tab )
   local parsed = {}
   local function doPrint(key, value, space)
      space = space or ''
      if type(value) == 'table' then
         if parsed[value] then
            print(space..key..' = <'..parsed[value]..'>')
         else
            parsed[value] = key
	    io.write( space, key, ' = {', "\n" )
	    space = space..'   '
	    foreach(value, function(key, value) doPrint(key, value, space) end)
	    io.write( space, "}", "\n" )
            end
      else
         if type(value) == 'string' then
	    value = string.format("%q",value)
            end
	 io.write(space, key, ' = ', tostring(value), "\n" )
         end
      end
   doPrint(name, tab)
   io.write("\n")
   end

require'serialize'

local function showt( tag, tb )
-- printValue         ( tag, tb )
   io.stdout:serialize( tag, tb )
   end

showt( "result1",json.decode[[
{
"accounting" : [
    {   "firstName" :   "John"  ,
        "lastName"  :   "Doe"   ,
        "age"       :   23      ,
    },
    {   "firstName" :   "Mary"  ,
        "lastName"  :   "Smith" ,
        "age"       :   32      ,
    }
],
"sales" : [
    {   "firstName" :   "Sally" ,
        "lastName"  :   "Green" ,
        "age"       :   27      ,
    },
    {   "firstName" :   "Jim"   ,
        "lastName"  :   "Galley",
        "age"       :   41      ,
    },
],
"numtest1" : 1e3,
"numtest2" : 1e3,
"numtest3" : 1.0e3,
"numtest4" : [ 77, "hello world" ],
"tellme" : [ "\u0021\n\r", "hello world", null, "three" ],
"lol" : null,
}
]] )

showt( "http://labs.adobe.com/technologies/spry/samples/data_region/JSONDataSetSample.html", json.decode[[
[
	{
		"id": "0001",
		"type": "donut",
		"name": "Cake",
		"ppu": 0.55,
		"batters":
			{
				"batter":
					[
						{ "id": "1001", "type": "Regular" },
						{ "id": "1002", "type": "Chocolate" },
						{ "id": "1003", "type": "Blueberry" },
						{ "id": "1004", "type": "Devil's Food" }
					]
			},
		"topping":
			[
				{ "id": "5001", "type": "None" },
				{ "id": "5002", "type": "Glazed" },
				{ "id": "5005", "type": "Sugar" },
				{ "id": "5007", "type": "Powdered Sugar" },
				{ "id": "5006", "type": "Chocolate with Sprinkles" },
				{ "id": "5003", "type": "Chocolate" },
				{ "id": "5004", "type": "Maple" }
			]
	},
	{
		"id": "0002",
		"type": "donut",
		"name": "Raised",
		"ppu": 0.55,
		"batters":
			{
				"batter":
					[
						{ "id": "1001", "type": "Regular" }
					]
			},
		"topping":
			[
				{ "id": "5001", "type": "None" },
				{ "id": "5002", "type": "Glazed" },
				{ "id": "5005", "type": "Sugar" },
				{ "id": "5003", "type": "Chocolate" },
				{ "id": "5004", "type": "Maple" }
			]
	},
	{
		"id": "0003",
		"type": "donut",
		"name": "Old Fashioned",
		"ppu": 0.55,
		"batters":
			{
				"batter":
					[
						{ "id": "1001", "type": "Regular" },
						{ "id": "1002", "type": "Chocolate" }
					]
			},
		"topping":
			[
				{ "id": "5001", "type": "None" },
				{ "id": "5002", "type": "Glazed" },
				{ "id": "5003", "type": "Chocolate" },
				{ "id": "5004", "type": "Maple" }
			]
	}
]
]] )

--[[
[
	{
		color: "red",
		value: "#f00"
	},
	{
		color: "green",
		value: "#0f0"
	},
	{
		color: "blue",
		value: "#00f"
	},
	{
		color: "cyan",
		value: "#0ff"
	},
	{
		color: "magenta",
		value: "#f0f"
	},
	{
		color: "yellow",
		value: "#ff0"
	},
	{
		color: "black",
		value: "#000"
	}
]
]]

showt( "http://www.json.org/example.html", json.decode[[
{"web-app": {
  "servlet": [
	 {
		"servlet-name": "cofaxCDS",
		"servlet-class": "org.cofax.cds.CDSServlet",
		"init-param": {
		  "configGlossary:installationAt": "Philadelphia, PA",
		  "configGlossary:adminEmail": "ksm@pobox.com",
		  "configGlossary:poweredBy": "Cofax",
		  "configGlossary:poweredByIcon": "/images/cofax.gif",
		  "configGlossary:staticPath": "/content/static",
		  "templateProcessorClass": "org.cofax.WysiwygTemplate",
		  "templateLoaderClass": "org.cofax.FilesTemplateLoader",
		  "templatePath": "templates",
		  "templateOverridePath": "",
		  "defaultListTemplate": "listTemplate.htm",
		  "defaultFileTemplate": "articleTemplate.htm",
		  "useJSP": false,
		  "jspListTemplate": "listTemplate.jsp",
		  "jspFileTemplate": "articleTemplate.jsp",
		  "cachePackageTagsTrack": 200,
		  "cachePackageTagsStore": 200,
		  "cachePackageTagsRefresh": 60,
		  "cacheTemplatesTrack": 100,
		  "cacheTemplatesStore": 50,
		  "cacheTemplatesRefresh": 15,
		  "cachePagesTrack": 200,
		  "cachePagesStore": 100,
		  "cachePagesRefresh": 10,
		  "cachePagesDirtyRead": 10,
		  "searchEngineListTemplate": "forSearchEnginesList.htm",
		  "searchEngineFileTemplate": "forSearchEngines.htm",
		  "searchEngineRobotsDb": "WEB-INF/robots.db",
		  "useDataStore": true,
		  "dataStoreClass": "org.cofax.SqlDataStore",
		  "redirectionClass": "org.cofax.SqlRedirection",
		  "dataStoreName": "cofax",
		  "dataStoreDriver": "com.microsoft.jdbc.sqlserver.SQLServerDriver",
		  "dataStoreUrl": "jdbc:microsoft:sqlserver://LOCALHOST:1433;DatabaseName=goon",
		  "dataStoreUser": "sa",
		  "dataStorePassword": "dataStoreTestQuery",
		  "dataStoreTestQuery": "SET NOCOUNT ON;select test='test';",
		  "dataStoreLogFile": "/usr/local/tomcat/logs/datastore.log",
		  "dataStoreInitConns": 10,
		  "dataStoreMaxConns": 100,
		  "dataStoreConnUsageLimit": 100,
		  "dataStoreLogLevel": "debug",
		  "maxUrlLength": 500}},
	 {
		"servlet-name": "cofaxEmail",
		"servlet-class": "org.cofax.cds.EmailServlet",
		"init-param": {
		"mailHost": "mail1",
		"mailHostOverride": "mail2"}},
	 {
		"servlet-name": "cofaxAdmin",
		"servlet-class": "org.cofax.cds.AdminServlet"},

	 {
		"servlet-name": "fileServlet",
		"servlet-class": "org.cofax.cds.FileServlet"},
	 {
		"servlet-name": "cofaxTools",
		"servlet-class": "org.cofax.cms.CofaxToolsServlet",
		"init-param": {
		  "templatePath": "toolstemplates/",
		  "log": 1,
		  "logLocation": "/usr/local/tomcat/logs/CofaxTools.log",
		  "logMaxSize": "",
		  "dataLog": 1,
		  "dataLogLocation": "/usr/local/tomcat/logs/dataLog.log",
		  "dataLogMaxSize": "",
		  "removePageCache": "/content/admin/remove?cache=pages&id=",
		  "removeTemplateCache": "/content/admin/remove?cache=templates&id=",
		  "fileTransferFolder": "/usr/local/tomcat/webapps/content/fileTransferFolder",
		  "lookInContext": 1,
		  "adminGroupID": 4,
		  "betaServer": true}}],
  "servlet-mapping": {
	 "cofaxCDS": "/",
	 "cofaxEmail": "/cofaxutil/aemail/*",
	 "cofaxAdmin": "/admin/*",
	 "fileServlet": "/static/*",
	 "cofaxTools": "/tools/*"},

  "taglib": {
	 "taglib-uri": "cofax.tld",
	 "taglib-location": "/WEB-INF/tlds/cofax.tld"}}}

]] )
