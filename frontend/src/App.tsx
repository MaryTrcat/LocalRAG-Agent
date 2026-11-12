import { AnimatePresence, motion, useReducedMotion } from 'framer-motion'
import {
  Check, ChevronDown, CircleHelp, Clock3, Command, FileInput, FileText, Menu, Plus, Search,
  Send, Settings2, Sparkles, Trash2, UploadCloud, X,
} from 'lucide-react'
import { useEffect, useRef, useState } from 'react'
import { GlassFolder, type FolderDocument } from './components/GlassFolder'

type ChatMessage = { id: number; role: 'user' | 'assistant'; text: string; time: string }
type HistoryItem = { id: string; title: string; time: string; active?: boolean; messages?: ChatMessage[] }
type DocumentPreview = { id: string; name: string; meta: string; content: string }

type KnowledgeFolder = {
  name: string
  eyebrow: string
  tone: 'blue' | 'amber' | 'violet'
  documents: FolderDocument[]
}

type LocalStats = { documents: number; chunks: number; messages: number; retrievalLogs: number }
type ApiSettings = { baseUrl: string; model: string; apiKeyEnv: string; apiKey: string }

const defaultApiSettings: ApiSettings = {
  baseUrl: 'https://api.deepseek.com/v1',
  model: 'deepseek-chat',
  apiKeyEnv: 'LOCALRAG_API_KEY',
  apiKey: '',
}

const readApiSettings = (): ApiSettings => {
  try {
    const saved = window.localStorage.getItem('localrag-api-settings')
    if (saved) {
      const parsed = JSON.parse(saved) as Partial<ApiSettings>
      return {
        baseUrl: typeof parsed.baseUrl === 'string' && parsed.baseUrl ? parsed.baseUrl : defaultApiSettings.baseUrl,
        model: typeof parsed.model === 'string' && parsed.model ? parsed.model : defaultApiSettings.model,
        apiKeyEnv: typeof parsed.apiKeyEnv === 'string' && parsed.apiKeyEnv ? parsed.apiKeyEnv : defaultApiSettings.apiKeyEnv,
        apiKey: typeof parsed.apiKey === 'string' ? parsed.apiKey : '',
      }
    }
  } catch {
    // Fall through to defaults and the legacy API key entry.
  }
  return { ...defaultApiSettings, apiKey: window.localStorage.getItem('localrag-api-key') ?? '' }
}

const removedExampleTitles = new Set([
  '请说明 LocalRAG 的检索链路',
  '如何降低 RAG 的无依据回答？',
  '切片长度和重叠应该怎么设置？',
  '本地索引会保存哪些数据？',
])

const folders: KnowledgeFolder[] = [{
  name: '知识资料库', eyebrow: 'LOCAL KNOWLEDGE', tone: 'blue', documents: [
    { id: 'prd', name: 'LocalRAG 产品定义', meta: '8.4 KB · 12 chunks', accent: '#73d3ff' },
    { id: 'flow', name: '问答链路流程', meta: '5.1 KB · 8 chunks', accent: '#9be3ff' },
    { id: 'arch', name: '检索架构说明', meta: '12.2 KB · 24 chunks', accent: '#8ccfff' },
    { id: 'api', name: 'API 接入备忘', meta: '4.7 KB · 6 chunks', accent: '#b5e6ff' },
    { id: 'rag', name: 'RAG 评估记录', meta: '18.9 KB · 31 chunks', accent: '#9fdcff' },
    { id: 'eval', name: '离线评测集', meta: '9.8 KB · 14 chunks', accent: '#d2f0ff' },
  ],
}]

const openingMessage: ChatMessage = { id: 1, role: 'assistant', time: '刚刚', text: '知识库已就绪。你可以从左下角设置里管理知识资料库、直接导入资料和配置 API，然后在这里继续提问。' }

const normalizeKnowledgeFolders = (saved: unknown): KnowledgeFolder[] => {
  const savedDocuments = Array.isArray(saved)
    ? saved.flatMap((folder) => {
      if (!folder || typeof folder !== 'object' || !Array.isArray((folder as { documents?: unknown }).documents)) return []
      return (folder as { documents: unknown[] }).documents.filter((document): document is FolderDocument => Boolean(document && typeof document === 'object' && 'id' in document && 'name' in document && 'meta' in document && 'accent' in document))
    })
    : []
  const documents = Array.from(new Map([...folders[0].documents, ...savedDocuments].map((document) => [document.id, document])).values())
  return [{ ...folders[0], documents }]
}

export default function App() {
  const reduceMotion = useReducedMotion()
  const [activeDoc, setActiveDoc] = useState('arch')
  const [query, setQuery] = useState('')
  const [historyQuery, setHistoryQuery] = useState('')
  const [historyOpen, setHistoryOpen] = useState(true)
  const historySearchRef = useRef<HTMLInputElement>(null)
  const composerRef = useRef<HTMLTextAreaElement>(null)
  const [messages, setMessages] = useState<ChatMessage[]>(() => {
    const saved = window.localStorage.getItem('localrag-messages')
    if (!saved) return [openingMessage]
    try {
      const parsed = JSON.parse(saved) as ChatMessage[]
      return parsed.some((message) => removedExampleTitles.has(message.text.trim())) ? [openingMessage] : parsed
    } catch {
      return [openingMessage]
    }
  })
  const [conversationId, setConversationId] = useState(() => window.localStorage.getItem('localrag-active-conversation') ?? `web-${Date.now()}`)
  const [hiddenHistoryIds, setHiddenHistoryIds] = useState<string[]>(() => {
    const saved = window.localStorage.getItem('localrag-hidden-history')
    if (!saved) return []
    try {
      const parsed = JSON.parse(saved) as unknown
      return Array.isArray(parsed) ? parsed.filter((id): id is string => typeof id === 'string') : []
    } catch {
      return []
    }
  })
  const [contextHistoryId, setContextHistoryId] = useState<string | null>(null)
  const [savedConversations, setSavedConversations] = useState<HistoryItem[]>(() => {
    const saved = window.localStorage.getItem('localrag-conversations')
    if (!saved) return []
    try {
      const parsed = JSON.parse(saved) as unknown
      return Array.isArray(parsed) ? parsed.filter((item): item is HistoryItem => Boolean(item && typeof item === 'object' && 'id' in item && 'title' in item && 'time' in item && !removedExampleTitles.has(String((item as { title?: unknown }).title ?? '').trim()) && Array.isArray((item as { messages?: unknown }).messages))) : []
    } catch {
      return []
    }
  })
  const [loading, setLoading] = useState(false)
  const [mobileMenu, setMobileMenu] = useState(false)
  const [settingsOpen, setSettingsOpen] = useState(false)
  const [baseUrl, setBaseUrl] = useState(() => readApiSettings().baseUrl)
  const [model, setModel] = useState(() => readApiSettings().model)
  const [apiKeyEnv, setApiKeyEnv] = useState(() => readApiSettings().apiKeyEnv)
  const [apiKey, setApiKey] = useState(() => readApiSettings().apiKey)
  const [apiSaved, setApiSaved] = useState(false)
  const [knowledgeFolders, setKnowledgeFolders] = useState(() => {
    const saved = window.localStorage.getItem('localrag-knowledge-folders')
    try {
      return normalizeKnowledgeFolders(saved ? JSON.parse(saved) : null)
    } catch {
      return folders
    }
  })
  const [libraryStats, setLibraryStats] = useState<LocalStats>({ documents: 0, chunks: 0, messages: 0, retrievalLogs: 0 })
  const [libraryError, setLibraryError] = useState('')
  const [importNotice, setImportNotice] = useState('')
  const [documentPreview, setDocumentPreview] = useState<DocumentPreview | null>(null)
  const recentHistory = (() => {
    const seen = new Set<string>()
    const currentMessages = messages.filter((message) => message.role === 'user')
    const currentConversation: HistoryItem[] = currentMessages.length > 0 ? [{ id: conversationId, title: currentMessages[0].text, time: currentMessages[currentMessages.length - 1].time, messages }] : []
    const questions: HistoryItem[] = [
      ...currentConversation,
      ...savedConversations,
    ]
    return questions.filter((item) => {
      const normalized = item.title.trim().toLocaleLowerCase()
      if (!normalized || seen.has(normalized) || hiddenHistoryIds.includes(item.id)) return false
      seen.add(normalized)
      return true
    }).slice(0, 40)
  })()
  const normalizedHistoryQuery = historyQuery.trim().toLocaleLowerCase().replace(/\s+/g, ' ')
  const filteredHistory = normalizedHistoryQuery
    ? recentHistory.filter((item) => item.title.toLocaleLowerCase().replace(/\s+/g, ' ').includes(normalizedHistoryQuery))
    : recentHistory

  useEffect(() => { window.localStorage.setItem('localrag-messages', JSON.stringify(messages)) }, [messages])
  useEffect(() => { window.localStorage.setItem('localrag-active-conversation', conversationId) }, [conversationId])
  useEffect(() => { window.localStorage.setItem('localrag-conversations', JSON.stringify(savedConversations)) }, [savedConversations])
  useEffect(() => { window.localStorage.setItem('localrag-hidden-history', JSON.stringify(hiddenHistoryIds)) }, [hiddenHistoryIds])
  useEffect(() => {
    const userMessages = messages.filter((message) => message.role === 'user')
    if (userMessages.length === 0) return
    setSavedConversations((current) => [{ id: conversationId, title: userMessages[0].text, time: userMessages[userMessages.length - 1].time, messages }, ...current.filter((item) => item.id !== conversationId)].slice(0, 40))
  }, [conversationId, messages])
  useEffect(() => { window.localStorage.setItem('localrag-knowledge-folders', JSON.stringify(knowledgeFolders)) }, [knowledgeFolders])
  useEffect(() => {
    void refreshLibrary()
  }, [])
  useEffect(() => {
    const handleSearchShortcut = (event: KeyboardEvent) => {
      if ((event.metaKey || event.ctrlKey) && event.key.toLowerCase() === 'k') {
        event.preventDefault()
        historySearchRef.current?.focus()
      }
      if (event.key === 'Escape' && document.activeElement === historySearchRef.current) {
        setHistoryQuery('')
        historySearchRef.current?.blur()
      }
    }
    window.addEventListener('keydown', handleSearchShortcut)
    return () => window.removeEventListener('keydown', handleSearchShortcut)
  }, [])

  const refreshLibrary = async () => {
    try {
      const [statsResponse, documentsResponse] = await Promise.all([fetch('/api/localrag/stats'), fetch('/api/localrag/documents')])
      const statsPayload = await statsResponse.json() as { stats?: LocalStats; error?: string }
      const documentsPayload = await documentsResponse.json() as { documents?: FolderDocument[]; error?: string }
      if (!statsResponse.ok || !documentsResponse.ok) throw new Error(statsPayload.error ?? documentsPayload.error ?? '无法读取本地库')
      setLibraryStats(statsPayload.stats ?? { documents: 0, chunks: 0, messages: 0, retrievalLogs: 0 })
      setKnowledgeFolders([{ ...folders[0], documents: documentsPayload.documents ?? [] }])
      setLibraryError('')
    } catch (error) {
      setLibraryError(error instanceof Error ? error.message : '无法连接本地库')
    }
  }

  const importLocalDocument = async (file: File) => {
    const extension = file.name.slice(file.name.lastIndexOf('.')).toLowerCase()
    if (!['.md', '.txt', '.docx'].includes(extension)) {
      setImportNotice('目前支持 .md、.txt 和 .docx 文件')
      return
    }
    try {
      let content: string
      if (extension === '.docx') {
        const bytes = new Uint8Array(await file.arrayBuffer())
        let binary = ''
        bytes.forEach((byte) => { binary += String.fromCharCode(byte) })
        content = btoa(binary)
      } else {
        content = await file.text()
      }
      const response = await fetch('/api/localrag/import', { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify({ name: file.name, content }) })
      const payload = await response.json() as { document?: FolderDocument; stats?: LocalStats; error?: string }
      if (!response.ok) throw new Error(payload.error ?? '导入失败')
      setLibraryStats(payload.stats ?? libraryStats)
      await refreshLibrary()
      setImportNotice(`已导入 1 条资料：${payload.document?.name ?? file.name}`)
    } catch (error) {
      setImportNotice(error instanceof Error ? error.message : '导入失败')
    }
    window.setTimeout(() => setImportNotice(''), 3200)
  }

  const deleteLocalDocument = async (document: FolderDocument) => {
    if (!window.confirm(`确定删除“${document.name}”吗？该文档的切片和检索记录也会删除。`)) return
    try {
      const response = await fetch('/api/localrag/delete', { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify({ id: document.id }) })
      const payload = await response.json() as { error?: string; stats?: LocalStats }
      if (!response.ok) throw new Error(payload.error ?? '删除失败')
      setLibraryStats(payload.stats ?? libraryStats)
      await refreshLibrary()
      setImportNotice(`已删除：${document.name}`)
    } catch (error) {
      setImportNotice(error instanceof Error ? error.message : '删除失败')
    }
    window.setTimeout(() => setImportNotice(''), 3200)
  }

  const clearLocalLibrary = async () => {
    if (!window.confirm('确定要清空本地库吗？已导入的文档、切片和问答记录都会删除。')) return
    try {
      const response = await fetch('/api/localrag/clear', { method: 'POST' })
      const payload = await response.json() as { stats?: LocalStats; error?: string }
      if (!response.ok) throw new Error(payload.error ?? '清空失败')
      setLibraryStats(payload.stats ?? { documents: 0, chunks: 0, messages: 0, retrievalLogs: 0 })
      setKnowledgeFolders([{ ...folders[0], documents: [] }])
      setMessages([openingMessage])
      setSavedConversations([])
      setConversationId(`web-${Date.now()}`)
      setImportNotice('本地库已清空')
    } catch (error) {
      setImportNotice(error instanceof Error ? error.message : '清空失败')
    }
    window.setTimeout(() => setImportNotice(''), 3200)
  }

  const sendQuestion = (questionText = query) => {
    const trimmed = questionText.trim()
    if (!trimmed || loading) return
    const now = new Date().toLocaleTimeString('zh-CN', { hour: '2-digit', minute: '2-digit' })
    setMessages((current) => [...current, { id: Date.now(), role: 'user', text: trimmed, time: now }])
    setQuery('')
    setLoading(true)
    window.setTimeout(() => {
      void fetch('/api/chat', { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify({ question: trimmed, baseUrl, model, apiKeyEnv, apiKey }) })
        .then(async (response) => {
          const payload = await response.json() as { answer?: string; stats?: LocalStats; error?: string }
          if (!response.ok) throw new Error(payload.error ?? '问答失败')
          setLibraryStats(payload.stats ?? libraryStats)
          setMessages((current) => [...current, { id: Date.now() + 1, role: 'assistant', time: '刚刚', text: payload.answer ?? '模型没有返回回答。' }])
        })
        .catch((error: unknown) => setMessages((current) => [...current, { id: Date.now() + 1, role: 'assistant', time: '刚刚', text: `问答失败：${error instanceof Error ? error.message : '本地服务异常'}` }]))
        .finally(() => setLoading(false))
    }, 900)
  }
  const resetChat = () => { setMessages([openingMessage]); setQuery(''); setConversationId(`web-${Date.now()}`); setContextHistoryId(null); setMobileMenu(false) }
  const openHistoryItem = (item: HistoryItem) => {
    setContextHistoryId(null)
    if (item.messages?.length) {
      setConversationId(item.id)
      setMessages(item.messages)
      setQuery('')
      setMobileMenu(false)
      return
    }
    setConversationId(item.id)
    setMessages([
      { id: Date.now(), role: 'user', text: item.title, time: item.time },
      { id: Date.now() + 1, role: 'assistant', text: '这条历史记录只有问题，尚未保存回答。', time: item.time },
    ])
    setQuery('')
    setMobileMenu(false)
    setImportNotice('这条历史记录只有问题，尚未保存回答')
    window.setTimeout(() => setImportNotice(''), 3200)
  }
  const deleteHistoryItem = (item: HistoryItem) => {
    setContextHistoryId(null)
    setHiddenHistoryIds((current) => current.includes(item.id) ? current : [...current, item.id])
    setSavedConversations((current) => current.filter((conversation) => conversation.id !== item.id))
    if (item.id === conversationId) resetChat()
  }
  const openLocalDocument = async (document: FolderDocument) => {
    try {
      const response = await fetch('/api/localrag/document', { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify({ id: document.id }) })
      const payload = await response.json() as { document?: { name?: string; meta?: string }; content?: string; error?: string }
      if (!response.ok) throw new Error(payload.error ?? '无法打开资料')
      setActiveDoc(document.id)
      setSettingsOpen(false)
      setDocumentPreview({ id: document.id, name: payload.document?.name ?? document.name, meta: payload.document?.meta ?? document.meta, content: payload.content ?? '' })
    } catch (error) {
      setImportNotice(error instanceof Error ? error.message : '无法打开资料')
    }
  }
  const saveApiSettings = () => {
    const settings: ApiSettings = { baseUrl: baseUrl.trim(), model: model.trim(), apiKeyEnv: apiKeyEnv.trim(), apiKey }
    window.localStorage.setItem('localrag-api-settings', JSON.stringify(settings))
    window.localStorage.setItem('localrag-api-key', apiKey)
    setBaseUrl(settings.baseUrl)
    setModel(settings.model)
    setApiKeyEnv(settings.apiKeyEnv)
    setApiSaved(true)
  }

  return <motion.div className="app-shell" initial={reduceMotion ? false : { opacity: 0 }} animate={{ opacity: 1 }} transition={{ duration: 0.6 }}>
    <div className="grain" />
    <header className="topbar"><div className="brand-lockup"><span className="brand-mark"><Sparkles size={15} /></span><span><strong>LOCALRAG</strong><small>证据工作台 / 01</small></span></div><div className="topbar-actions"><button className="icon-button mobile-menu-button" onClick={() => setMobileMenu((value) => !value)} aria-label="打开导航"><Menu size={18} /></button></div></header>
    <div className="workspace-grid">
      <motion.aside className={`sidebar ${mobileMenu ? 'mobile-open' : ''}`} initial={reduceMotion ? false : { x: -24, opacity: 0 }} animate={{ x: 0, opacity: 1 }} transition={{ duration: 0.65, delay: 0.08 }}>
        <div className="sidebar-intro"><div className="eyebrow">QUESTION HISTORY</div><h1>每个问题<br /><em>都有来处。</em></h1><p>从最近的对话继续思考，或打开设置管理你的本地知识库。</p></div>
        <button className="new-session-button" onClick={resetChat}><Command size={16} /> <span>新会话</span></button>
        <div className="side-section-heading"><button className="history-dropdown-trigger" aria-expanded={historyOpen} onClick={() => setHistoryOpen((value) => !value)}><span>{normalizedHistoryQuery ? `搜索结果 · ${filteredHistory.length}` : '最近问过'}</span><ChevronDown size={15} className={historyOpen ? 'dropdown-chevron open' : 'dropdown-chevron'} /></button><button className="mini-button" aria-label="新建问题" onClick={resetChat}><Plus size={15} /></button></div>
        <AnimatePresence initial={false}>{historyOpen && <motion.div className="history-list" variants={{ show: { transition: { staggerChildren: 0.07 } } }} initial="hide" animate="show" exit={{ opacity: 0, height: 0, overflow: 'hidden' }}>{filteredHistory.map((item) => <motion.div key={item.id} className="history-item-wrap" variants={{ hide: { opacity: 0, x: -12 }, show: { opacity: 1, x: 0 } }} onContextMenu={(event) => { event.preventDefault(); setContextHistoryId(item.id) }}><motion.button className={`history-item ${item.id === conversationId || item.active ? 'active' : ''}`} onClick={() => openHistoryItem(item)}><span className="history-icon"><FileText size={13} /></span><span className="history-copy"><strong>{item.title}</strong><small>{item.time}</small></span>{(item.id === conversationId || item.active) && <span className="history-live" />}</motion.button>{contextHistoryId === item.id && <motion.div className="history-context-menu" role="menu" initial={{ opacity: 0, y: -4 }} animate={{ opacity: 1, y: 0 }} onClick={(event) => event.stopPropagation()}><button role="menuitem" onClick={() => deleteHistoryItem(item)}><Trash2 size={13} /> 删除记录</button></motion.div>}</motion.div>)}{filteredHistory.length === 0 && <div className="history-empty">没有找到相关问题</div>}</motion.div>}</AnimatePresence>
        <div className="sidebar-footer"><div className="search-field"><Search size={16} /><input ref={historySearchRef} value={historyQuery} onChange={(event) => setHistoryQuery(event.target.value)} placeholder="搜索历史问题" aria-label="搜索历史问题" />{historyQuery ? <button className="search-clear" aria-label="清除搜索" onClick={() => { setHistoryQuery(''); historySearchRef.current?.focus() }}><X size={13} /></button> : <kbd>Ctrl K</kbd>}</div><p className="library-summary">本地库：{libraryStats.documents} 份资料 · {libraryStats.chunks} 个片段</p><button className="settings-trigger" onClick={() => { setSettingsOpen(true); setMobileMenu(false) }}><Settings2 size={16} /><span><strong>设置</strong><small>知识资料库 · 导入 · API</small></span><span className="settings-arrow">↗</span></button></div>
      </motion.aside>
      <main className="conversation-pane">
        <motion.div className="conversation-head" initial={reduceMotion ? false : { y: -14, opacity: 0 }} animate={{ y: 0, opacity: 1 }} transition={{ delay: 0.18, duration: 0.55 }} />
        <div className="message-scroll" aria-live="polite"><AnimatePresence initial={!reduceMotion} mode="popLayout">{messages.map((message) => <motion.article key={message.id} className={`message ${message.role}`} initial={reduceMotion ? false : { opacity: 0, y: 18, scale: 0.98 }} animate={{ opacity: 1, y: 0, scale: 1 }} exit={{ opacity: 0, y: -8 }} transition={{ type: 'spring', stiffness: 260, damping: 24 }}><div className="message-meta"><span className="avatar">{message.role === 'assistant' ? <Sparkles size={13} /> : '你'}</span><strong>{message.role === 'assistant' ? 'LOCALRAG' : 'YOU'}</strong><time>{message.time}</time>{message.role === 'assistant' && <span className="verified"><Check size={11} /> 已引用</span>}</div><p>{message.text}</p></motion.article>)}</AnimatePresence><AnimatePresence>{loading && <motion.div className="thinking-line" initial={{ opacity: 0 }} animate={{ opacity: 1 }} exit={{ opacity: 0 }}><span className="thinking-dots"><i /><i /><i /></span> 正在检索本地资料…</motion.div>}</AnimatePresence></div>
        <motion.div className="composer-wrap" initial={reduceMotion ? false : { y: 22, opacity: 0 }} animate={{ y: 0, opacity: 1 }} transition={{ delay: 0.4, duration: 0.6 }}><div className="composer"><textarea ref={composerRef} value={query} onChange={(event) => setQuery(event.target.value)} onKeyDown={(event) => { if (event.key === 'Enter' && !event.shiftKey) { event.preventDefault(); sendQuestion() } }} placeholder="问问你的本地资料…" aria-label="问题输入框" rows={2} /><div className="composer-bottom"><span><CircleHelp size={14} /> Enter 发送 · Shift + Enter 换行</span><motion.button className="send-button" onClick={() => sendQuestion()} whileHover={{ y: -2 }} whileTap={{ scale: 0.96 }} disabled={loading || !query.trim()} aria-label="发送问题"><Send size={16} /></motion.button></div></div><div className="composer-note"><span><Clock3 size={13} /> 最近索引于 2 分钟前更新</span><span>Local only · 不上传原始文件</span></div></motion.div>
      </main>
    </div>
    <AnimatePresence>{settingsOpen && <motion.div className="settings-overlay" initial={{ opacity: 0 }} animate={{ opacity: 1 }} exit={{ opacity: 0 }} onClick={() => setSettingsOpen(false)}><motion.section className="settings-panel" initial={reduceMotion ? false : { opacity: 0, y: 18, scale: .98 }} animate={{ opacity: 1, y: 0, scale: 1 }} exit={{ opacity: 0, y: 12, scale: .98 }} transition={{ type: 'spring', stiffness: 240, damping: 24 }} onClick={(event) => event.stopPropagation()}><div className="settings-head"><div><div className="eyebrow">WORKSPACE SETTINGS</div><h3>设置</h3><p>知识资料库、直接导入和模型连接都集中在这里。</p></div><button className="icon-button" onClick={() => setSettingsOpen(false)} aria-label="关闭设置"><X size={17} /></button></div><div className="settings-scroll"><section className="settings-section"><div className="settings-section-head"><div><span className="settings-kicker">01 · KNOWLEDGE</span><h4>知识资料库</h4></div><label className="quiet-button import-label"><UploadCloud size={14} /> 导入相关资料<input type="file" accept=".md,.txt,.docx,text/markdown,text/plain,application/vnd.openxmlformats-officedocument.wordprocessingml.document" aria-label="导入本地资料" onChange={(event) => { const file = event.target.files?.[0]; if (file) void importLocalDocument(file); event.currentTarget.value = '' }} /></label></div>{importNotice && <motion.div className="import-notice" initial={{ opacity: 0, y: -5 }} animate={{ opacity: 1, y: 0 }}><Check size={14} /> {importNotice}</motion.div>}{libraryError && <div className="import-error">{libraryError}</div>}<div className="folder-stack settings-folders">{knowledgeFolders.map((folder) => <GlassFolder {...folder} key={folder.name} count={folder.documents.length} activeDocument={activeDoc} onOpenDocument={openLocalDocument} onDeleteDocument={deleteLocalDocument} />)}</div><div className="settings-danger"><button className="danger-button" onClick={() => void clearLocalLibrary}>清空本地库</button><span>删除 SQLite 中的资料、切片和检索记录</span></div></section><section className="settings-section"><div className="settings-section-head"><div><span className="settings-kicker">02 · CONNECTION</span><h4>API 连接</h4></div><span className={`save-state ${apiSaved ? 'saved' : ''}`}>{apiSaved ? '已保存' : '本地配置'}</span></div><div className="settings-form"><label>API Base URL<input value={baseUrl} onChange={(event) => setBaseUrl(event.target.value)} /></label><label>聊天模型<input value={model} onChange={(event) => setModel(event.target.value)} /></label><label>API Key 环境变量<input value={apiKeyEnv} onChange={(event) => setApiKeyEnv(event.target.value)} /></label><label>API Key（仅本机）<input type="password" value={apiKey} onChange={(event) => setApiKey(event.target.value)} placeholder="sk-..." aria-label="API Key（仅本机）" /></label></div><div className="settings-actions"><span><FileInput size={14} /> Key 仅发送给本机服务，不上传到 LocalRAG</span><button className="save-button" onClick={saveApiSettings}>保存连接 <Check size={14} /></button></div></section></div></motion.section></motion.div>}</AnimatePresence>
    <AnimatePresence>{documentPreview && <motion.div className="document-preview-overlay" initial={{ opacity: 0 }} animate={{ opacity: 1 }} exit={{ opacity: 0 }} onClick={() => setDocumentPreview(null)}><motion.section className="document-preview-panel" initial={reduceMotion ? false : { opacity: 0, y: 18, scale: .98 }} animate={{ opacity: 1, y: 0, scale: 1 }} exit={{ opacity: 0, y: 12, scale: .98 }} transition={{ type: 'spring', stiffness: 240, damping: 24 }} onClick={(event) => event.stopPropagation()} role="dialog" aria-modal="true" aria-label={`查看资料 ${documentPreview.name}`}><div className="document-preview-head"><div><div className="eyebrow">LOCAL DOCUMENT</div><h3>{documentPreview.name}</h3><p>{documentPreview.meta}</p></div><button className="icon-button" onClick={() => setDocumentPreview(null)} aria-label="关闭资料预览"><X size={17} /></button></div><pre className="document-preview-content">{documentPreview.content}</pre></motion.section></motion.div>}</AnimatePresence>
  </motion.div>
}
