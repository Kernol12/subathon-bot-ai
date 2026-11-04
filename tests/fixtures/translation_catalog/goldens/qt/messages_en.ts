<?xml version="1.0" encoding="utf-8"?>
<TS version="2.1" language="en" sourcelanguage="en">
  <context>
    <name></name>
    <message id="fixture.greeting">
      <source>Hello, {name}!</source>
      <extracomment>Fixture greeting; name is untrusted plain text.</extracomment>
      <translation>Hello, {name}!</translation>
    </message>
    <message id="fixture.items" numerus="yes">
      <source>{count} items for {name}</source>
      <extracomment>Fixture count with reordered named values.</extracomment>
      <translation>
        <numerusform>{count} item for {name}</numerusform>
        <numerusform>{count} items for {name}</numerusform>
      </translation>
    </message>
    <message id="fixture.literal">
      <source>{{x}} %n %1 %L1 %% {name}</source>
      <extracomment>Literal braces and Qt-looking percent sequences.</extracomment>
      <translation>{{x}} %n %1 %L1 %% {name}</translation>
    </message>
    <message id="fixture.reorder">
      <source>{first} then {second}; {first}</source>
      <extracomment>Named values reorder and repeat across languages.</extracomment>
      <translation>{first} then {second}; {first}</translation>
    </message>
    <message id="fixture.scalars">
      <source>A &amp; B &lt; C &gt; D &quot;q&quot; &apos;s&apos;&#13;
	Grüße é 😀</source>
      <extracomment>Exact XML scalars and line controls.</extracomment>
      <translation>A &amp; B &lt; C &gt; D &quot;q&quot; &apos;s&apos;&#13;
	Grüße é 😀</translation>
    </message>
    <message id="fixture.selector_only" numerus="yes">
      <source>Items for {name}</source>
      <extracomment>Selector is declared even when absent from visible text.</extracomment>
      <translation>
        <numerusform>One item for {name}</numerusform>
        <numerusform>Items for {name}</numerusform>
      </translation>
    </message>
  </context>
</TS>
