import "./ChatRoom.css";
import React, { useEffect, useState, useRef } from "react";
import { useParams, useNavigate } from "react-router-dom";

const ChatRoom = () => {
  const { username } = useParams(); // the user you're chatting with
  const navigate = useNavigate();
  const currentUser = localStorage.getItem("username");
  const [messages, setMessages] = useState([]);
  const [text, setText] = useState("");
  const ws = useRef(null);

  useEffect(() => {
    // Replace with THE ACTUAL BACKEND WEBSOCKET URL
    ws.current = new WebSocket("ws://localhost:9000");

    ws.current.onopen = () => {
      ws.current.send(JSON.stringify({ type: "login", username: currentUser }));
    };

    ws.current.onmessage = (message) => {
      const data = JSON.parse(message.data);
      if (
        (data.from === currentUser && data.to === username) ||
        (data.from === username && data.to === currentUser)
      ) {
        setMessages((prev) => [...prev, data]);
      }
    };

    return () => ws.current?.close();
  }, [username, currentUser]);

  const sendMessage = () => {
    if (!text.trim()) return;

    const msg = {
      type: "message",
      from: currentUser,
      to: username,
      content: text,
    };
    ws.current.send(JSON.stringify(msg));
    setMessages((prev) => [...prev, msg]);
    setText("");
  };

  return (
    <div className="chatroom-container">
      <div className="chatroom-header">
        <button className="back-button" onClick={() => navigate("/dashboard")}>
          ← Back
        </button>
        <h2>Chatting with {username}</h2>
      </div>

      <div className="chat-window">
        {messages.map((msg, i) => (
          <div
            key={i}
            className={`chat-bubble ${
              msg.from === currentUser ? "sent" : "received"
            }`}
          >
            <p>{msg.content}</p>
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
