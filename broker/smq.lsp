<?lsp

-- SMQ entry point. URL to this file is localhost/smq.lsp
-- The code below calls broker:connect()
-- See: https://realtimelogic.com/ba/doc/?url=SMQ.html#connect

if app.smq.isSMQ(request) then
   trace"New SMQ Client Request"
   -- Upgrade HTTP(S) request to SMQ connection
   app.broker:connect(request)
else -- Not an SMQ client
   response:senderror(400, "Not an SMQ Connection Request!")
end
?>

