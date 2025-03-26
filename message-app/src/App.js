import React from "react";
import { BrowserRouter as Router, Routes, Route } from "react-router-dom";
import LoginPage from "./pages/LoginPage";
import ChatDashboard from "./pages/ChatDashboard";
import ChatRoom from "./pages/ChatRoom";

function App() {
  return (
    <Router>
      <Routes>
        <Route path="/" element={<LoginPage />} />
        <Route path="/dashboard" element={<ChatDashboard />} />
        <Route path="/chat/:username" element={<ChatRoom />} />
      </Routes>
    </Router>
  );
}

export default App;
