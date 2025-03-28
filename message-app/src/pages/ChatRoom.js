import "./ChatRoom.css";
import React, { useEffect, useState, useRef } from "react";
import { useParams, useNavigate } from "react-router-dom";

const ChatRoom = () => {
  const { username: room } = useParams(); // 'username' is actually the room name
  const navigate = useNavigate();
  const currentUser = localStorage.getItem("username");
  const [messages, setMessages] = useState([]);
  const [text, setText] = useState("");
  const ws = useRef(null);

  useEffect(() => {
    ws.current = new WebSocket("ws://localhost:9000");

    ws.current.onopen = () => {
      const joinMessage = {
        type: "join", // Changed from "login" to "join"
        username: currentUser,
        room: room,
      };
      ws.current.send(JSON.stringify(joinMessage));
      console.log("Sent join message:", joinMessage);
    };

    ws.current.onmessage = (message) => {
      const data = JSON.parse(message.data);

      // Accept only messages for this room
      if (data.type === "message" && data.room === room) {
        setMessages((prev) => [...prev, data]);
      }
    };

    ws.current.onerror = (err) => {
      console.error("WebSocket error:", err);
    };

    ws.current.onclose = () => {
      console.log("WebSocket closed");
    };

    return () => ws.current?.close();
  }, [room, currentUser]);

  const sendMessage = () => {
    if (!text.trim()) return;

    const messageToSend = {
      type: "message",
      from: currentUser,
      room: room,
      content: text,
      timestamp: new Date().toISOString(),
    };

    if (ws.current && ws.current.readyState === WebSocket.OPEN) {
      console.log("Sending message:", messageToSend);
      ws.current.send(JSON.stringify(messageToSend));
      setText("");
    } else {
      console.warn("WebSocket is not open. Cannot send message.");
    }
  };

  return (
      <div className="chatroom-container">
        <div className="chatroom-header">
          <button className="back-button" onClick={() => navigate("/dashboard")}>
            ← Back
          </button>
          <h2>Room: {room}</h2>
        </div>

        <div className="chat-window">
          {messages.map((msg, i) => (
              <div
                  key={i}
                  className={`message-block ${
                      msg.from === currentUser ? "sent-block" : "received-block"
                  }`}
              >
                {msg.from !== currentUser && (
                    <span className="sender-name">{msg.from}</span>
                )}
                <div
                    className={`chat-bubble ${
                        msg.from === currentUser ? "sent" : "received"
                    }`}
                >
                  <p>{msg.content}</p>
                </div>
                {msg.timestamp && (
                    <span className="timestamp">
                {new Date(msg.timestamp).toLocaleTimeString([], {
                  hour: "numeric",
                  minute: "2-digit",
                  hour12: true,
                })}
              </span>
                )}
              </div>
          ))}
        </div>

        <div className="message-input-area">
          <input
              className="message-input"
              value={text}
              onChange={(e) => setText(e.target.value)}
              placeholder="Type your message..."
          />
          <button className="send-button" onClick={sendMessage}>
            Send
          </button>
        </div>
      </div>
  );
};

export default ChatRoom;
