import React from "react";
import { useNavigate } from "react-router-dom";

const users = ["alice", "bob", "charlie"]; // Replace with backend list later

const ChatDashboard = () => {
  const navigate = useNavigate();

  const handleChatClick = (username) => {
    navigate(`/chat/${username}`);
  };

  return (
    <div>
      <h2>Welcome, {localStorage.getItem("username")}</h2>
      <h3>Start a chat with:</h3>
      <ul>
        {users.map((user) => (
          <li key={user}>
            <button onClick={() => handleChatClick(user)}>{user}</button>
          </li>
        ))}
      </ul>
    </div>
  );
};

export default ChatDashboard;
